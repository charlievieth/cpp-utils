#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
#include <chrono>
#include <cerrno>
#include <stdexcept>

#include <filesystem>
#include <utility>
namespace fs = std::filesystem;

#include <sys/sysctl.h> // sysctl (for boot time)
#include <sys/time.h>   // timeval
#include <getopt.h>     // getopt_long

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include <SQLiteCpp/Database.h>

// WARN
#include <CLI/CLI.hpp>

#include <sqlite3.h>

// TODO: use or remove
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

constexpr const char *HISTDB_PROD = "HISTDB_PROD";
constexpr std::string_view HISTDB_NAME = "histdb.sqlite3";

constexpr int32_t BUSY_TIMEOUT_MS = 300;

// Options
////////////////////////////////////////////////////////////////////////////////

static const int current_schema_migration = 2;

constexpr char m001_create_tables_stmt[] = R"""(
BEGIN;

CREATE TABLE IF NOT EXISTS session_ids (
    id        INTEGER PRIMARY KEY,
    ppid      INTEGER NOT NULL,
    boot_time TIMESTAMP NOT NULL
);

CREATE TABLE IF NOT EXISTS history (
    `id`          INTEGER PRIMARY KEY,
    `session_id`  INTEGER NOT NULL,
    `history_id`  INTEGER NOT NULL,
    `ppid`        INTEGER NOT NULL,
    `status_code` INTEGER NOT NULL,
    `created_at`  TIMESTAMP NOT NULL,
    `username`    TEXT NOT NULL,
    `directory`   TEXT NOT NULL,
    `raw`         TEXT NOT NULL,
    FOREIGN KEY(session_id) REFERENCES session_ids(id)
);

CREATE TABLE IF NOT EXISTS schema_migrations (
	`version` INTEGER PRIMARY KEY
);

INSERT OR IGNORE INTO schema_migrations (version) VALUES (1);

COMMIT;
)""";

// TODO: use boot_ids table !!!
constexpr char m002_create_boot_id_table[] = R"""(
BEGIN;
CREATE TABLE IF NOT EXISTS boot_ids (
    id         INTEGER PRIMARY KEY,
    created_at TIMESTAMP NOT NULL
);
COMMIT;
INSERT OR IGNORE INTO schema_migrations (version) VALUES (2);
)""";

constexpr char insert_history_stmt[] = R"""(
INSERT INTO history (
	session_id,
	history_id,
	ppid,
	status_code,
	created_at,
	username,
	directory,
	raw
) VALUES (?, ?, ?, ?, ?, ?, ?, ?);
)""";

constexpr std::string_view root_usage_msg = R"""(histdb: shell history tool

Usage:
  histdb [command]

Available Commands:
  session: return a new session id
  insert:  insert a history entry
  info:    print information about the histdb environment

Use "histdb [command] --help" for more information about a command.
)""";

constexpr std::string_view insert_help_msg = R"""(insert a shell command into the histdb

Usage:
  histdb insert [flags] [history_id] [raw_command]

Flags:
    -d, --debug          print debug output (not supported)
    -s, --session        session id (required)
    -c, --status-code    command status/exit code (required)
        --dry-run        perform a "dry run" (not supported)
    -h, --help           help for insert
)""";

constexpr std::string_view session_help_msg = R"""(initiate a new histdb session

Usage:
  histdb session [flags]

Flags:
    -e, --eval    print the session id as a statment that can be evaluated by bash
    -h, --help    help for insert
)""";

constexpr std::string_view boot_id_help_msg = R"""(initiate a new histdb boot id

Usage:
  histdb boot-id [flags]

Flags:
    -e, --eval    print the session id as a statment that can be evaluated by bash
    -h, --help    help for insert
)""";

// Error handling
////////////////////////////////////////////////////////////////////////////////

#ifdef SQLITECPP_ENABLE_ASSERT_HANDLER
namespace SQLite
{
// definition of the assertion handler enabled when SQLITECPP_ENABLE_ASSERT_HANDLER is defined in the project (CMakeList.txt)
void assertion_failed(const char* apFile, const long apLine, const char* apFunc,
					  const char* apExpr, const char* apMsg) {
    // Print a message to the standard error output stream, and abort the program.
    std::cerr << apFile << ":" << apLine << ":" << " error: assertion failed (" << apExpr << ") in " << apFunc << "() with message \"" << apMsg << "\"\n";
    std::abort();
}
} /* SQLite */
#endif /* SQLITECPP_ENABLE_ASSERT_HANDLER */

// insert command options

static bool verbose = false;
// TODO: use this
static bool FORCE_USE_PROD_DATABASE = false;
static bool use_prod_database = false;
static bool print_usage = false;
static bool dry_run = false;
static int status_code = 0;
static long long session_id = 0;
static long long history_id = 0;
static std::string raw_history;
static std::string current_wd;
static std::string current_user;

static int opt_val;
static const struct option insert_cmd_opts[] = {
	{"debug",       no_argument,       nullptr, 'd'},
	{"help",        no_argument,       nullptr, 'h'},
	{"session",     required_argument, nullptr, 's'},
	{"status-code", required_argument, nullptr, 'c'},
	{"dry-run",     no_argument,       &opt_val, 1}, // TODO: remove if not used
	{nullptr,       0,                 nullptr,  0}, // zero pad end
};

// session command options

static bool print_eval = false;
static const struct option session_cmd_opts[] = {
	{"boot-id", required_argument, nullptr, 'b'},
	{"help",    no_argument,       nullptr, 'h'},
	{"eval",    no_argument,       nullptr, 'e'}, // print evalable output
	{nullptr,   0,                 nullptr, 0},   // zero pad end
};

// expections (TODO: move to separate file)

class ErrnoException : public std::runtime_error {
public:
	ErrnoException(const std::string& message) : std::runtime_error(message) {}
};

class ArgumentException : public std::invalid_argument {
public:
	ArgumentException(const std::string& message) : std::invalid_argument(message) {}
};

// usage helpers
// TODO: remove these

// static void print_usage_message(const std::string_view msg, std::ostream& out = std::cout) {
// 	if (auto n = msg.length(); n > 0 && msg[n-1] == '\n') {
// 		out << msg;
// 	} else {
// 		out << msg << std::endl;
// 	}
// }

static void root_usage(std::ostream& out = std::cout)    { out << root_usage_msg; }
static void session_usage(std::ostream& out = std::cout) { out << session_help_msg; }
static void boot_id_usage(std::ostream& out = std::cout) { out << boot_id_help_msg; }
static void insert_usage(std::ostream& out = std::cout)  { out << insert_help_msg; }

// parse helpers

static std::string_view safe_getenv(const char *name) {
	return absl::NullSafeStringView(std::getenv(name));
}

static constexpr bool parse_bool(std::string_view s) {
	if (s == "1" || s == "t" || s == "T" || s == "true" || s == "True" || s == "TRUE") {
		return true;
	}
	if (s == "0" || s == "f" || s == "F" || s == "false" || s == "False" || s == "FALSE") {
		return false;
	}
	throw ArgumentException(absl::StrCat("parse_bool: invalid argument: '", s, "'"));
}

static bool get_env_bool(const char *name) {
	auto val = safe_getenv(name);
	return !val.empty() && parse_bool(val);
}

static std::string must_getenv(const char *env_var) {
	auto val = safe_getenv(env_var);
	if (unlikely(val.empty())) {
		throw ArgumentException(absl::StrCat("empty environment variable: ", env_var));
	}
	return std::string(val);
}

static bool use_test_database() {
	if (FORCE_USE_PROD_DATABASE) {
		return false;
	}
	return get_env_bool(HISTDB_PROD) == false;
}

static constexpr bool is_ascii_space(unsigned char c) {
	return c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == ' ';
}

static constexpr bool is_ascii_cntrl(unsigned char c) {
	return c <= '\x1f' || c == '\x7F';
}

static constexpr std::string_view ascii_space_and_cntrl() {
	return "\x00\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r"
		"\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18"
		"\x19\x1a\x1b\x1c\x1d\x1e\x1f \x7f";
}

static std::string trim_ascii_space_x(const std::string& s)  {
	auto start = s.find_first_not_of("\t\n\v\f\r ");
	auto end = s.find_last_not_of(ascii_space_and_cntrl());
	if (start == std::string_view::npos || end == std::string_view::npos) {
		return std::string();
	}
	return s.substr(start, end);
}

static std::string trim_ascii_space(const char *s) {
	while (is_ascii_space(*s)) {
		s++;
	};
	if (*s) {
		ssize_t n = std::strlen(s) - 1;
		const unsigned char c = s[n];
		while (n >= 0 && (is_ascii_space(c) || is_ascii_cntrl(c))) {
			n--;
		}
		return std::string(s, n + 1);
	}
	return std::string();
}

// command line parsers

static bool chars_are_numeric(const char *s) {
	const unsigned char *p = (const unsigned char *)s;
	if (!p || *p == '\0') {
		return false;
	}
	unsigned char c;
	while ((c = *p++)) {
		if (!('0' <= c && c <= '9')) {
			return false;
		}
	}
	return true;
}

static void parse_insert_cmd_argments(int argc, char * const argv[]) {
	int ch;
	int opt_index = 0;
	bool status_set = false;
	bool session_set = false;
	while ((ch = getopt_long(argc, argv, "dhs:c:", insert_cmd_opts, &opt_index)) != -1) {
		switch (ch) {
		case 'd':
			// WARN: not supported
			std::cerr << "WARN: --debug is not supported" << std::endl;
			verbose = true;
			break;
		case 'h':
			print_usage = true;
			break;
		case 's':
			if (!chars_are_numeric(optarg)) {
				throw ArgumentException(absl::StrCat(
					"session id must be an iteger: '", optarg, "'"
				));
			}
			session_id = std::strtoll(optarg, nullptr, 10);
			if (session_id <= 0) {
				throw ArgumentException(absl::StrCat("non-positive session: ", session_id));
			}
			session_set = true;
			break;
		case 'c':
			// TODO: check for any exceptions
			if (!chars_are_numeric(optarg)) {
				throw ArgumentException(absl::StrCat(
					"session-code must be an iteger: '", optarg, "'"
				));
			}
			status_code = std::stoi(optarg, nullptr, 10);
			status_set = true;
			break;
		case 0:
			switch (opt_val) {
			case 1:
				dry_run = true;
				std::cerr << "--dry-run is not supported";
				break;
			default:
				throw ArgumentException(absl::StrCat("invalid argument: ", opt_val));
				break;
			}
		default:
			throw ArgumentException(absl::StrCat("invalid argument: ", ch));
			break;
		}
	}
	if (print_usage) {
		return;
	}
	if (!status_set || !session_set) {
		// TODO: check for any exceptions
		auto missing = std::string("missing required arguments:");
		if (!status_set) {
			missing += " --status-code";
		}
		if (!session_set) {
			missing += " --session";
		}
		throw ArgumentException(missing);
	}

	argc -= optind;
	argv += optind;
	if (argc != 1) {
		// std::cerr << "ARG: " << argv[0] << std::endl;
		throw ArgumentException(absl::StrCat(
			"expected 1 argument ([HISTORY_ID RAW_COMMAND]) got: ", argc
		));
	}

	char *p_end;
	char *raw = argv[0];
	history_id = std::strtoll(raw, &p_end, 10);
	if (history_id <= 0) {
		throw ArgumentException(absl::StrCat("non-positive: HISTORY_ID: ", history_id));
	}
	raw = p_end;
	if (!raw || !*raw) {
		throw ArgumentException("empty argument: RAW_HISTORY");
	}
	raw_history = trim_ascii_space(raw);

	// TODO: clean PWD via .lexically_normal()
	//
	// TODO: use getwd() to get the absolute WD since PWD can be wrong
	// or on macOS the casing can differ from the actual WD.
	current_wd = must_getenv("PWD");
	current_user = must_getenv("USER");;
}

static void parse_session_cmd_argments(int argc, char * const argv[]) {
	if (argc == 0) {
		return;
	}
	int ch;
	int opt_index = 0;
	// getopt_long is bad and this is kinda broken for --eval
	while ((ch = getopt_long(argc, argv, "he", session_cmd_opts, &opt_index)) != -1) {
		switch (ch) {
		case 'h':
			print_usage = true;
			break;
		case 'e':
			print_eval = true;
			break;
		default:
			throw ArgumentException(absl::StrCat("invalid argument: ", ch));
			break;
		}
	}
	if (print_usage) {
		return;
	}
	argc -= optind;
	argv += optind;
	if (argc != 0) {
		std::vector<std::string_view> extra;
		for (int i = 0; i < argc; i++) {
			extra.push_back(std::string_view(argv[i]));
		}
		throw ArgumentException(
			absl::StrCat("unexpected arguments: ", absl::StrJoin(extra, ", "))
		);
	}
}

static void parse_boot_id_cmd_argments(int argc, char * const argv[]) {
	parse_session_cmd_argments(argc, argv);
}

static void parse_dump_history_cmd_argments(int argc, char * const argv[]) {
	parse_session_cmd_argments(argc, argv);
}

////////////////////////////////////////////////////////////////////////////////

static fs::path user_data_dir() {
	auto s = safe_getenv("XDG_DATA_HOME");
	if (!s.empty()) {
		return s;
	}
	s = safe_getenv("HOME");
	if (!s.empty()) {
		return fs::path(s) / ".local" / "share";
	}
	throw ArgumentException("neither $XDG_DATA_HOME nor $HOME are defined");
}

static fs::path histdb_database_path() {
	// Pedantically guard against writing to the real database.
	// TODO: Remove this once testing is done.
	if (use_test_database()) {
		return "test.sqlite3";
	}
	return user_data_dir() / "histdb" / "data" / HISTDB_NAME;
}

static bool should_migrate_database(SQLite::Database& db) {
	if (!db.tableExists("schema_migrations")) {
		return true;
	}
	SQLite::Statement query(
		db, "SELECT version FROM schema_migrations ORDER BY version DESC LIMIT 1;"
	);
	if (!query.executeStep()) {
		return true;
	}
	int version = query.getColumn(0).getInt();

	// The database is running a schema version that we don't know about.
	if (unlikely(version > current_schema_migration)) {
		throw std::runtime_error(absl::StrCat(
			"database schema (", version, ") exceeds program ",
			"version (", current_schema_migration, ")"
		));
	}

	return version < current_schema_migration;
}

static SQLite::Database open_database(std::string& filename, bool readonly = false) {
	// TODO: set SQLITE_OPEN_EXRESCODE (if defined)
	const int flags = readonly ?
		SQLite::OPEN_READONLY :
		SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE;

	SQLite::Database db = SQLite::Database(filename, flags);

	// Enable extended error codes
	sqlite3_extended_result_codes(db.getHandle(), 1);

	// TODO: increase `PRAGMA main.cache_size;` for queries (-8000 is ~7.8mb)
	// TODO: use a 'PENDING' lock for writing and a 'SHARED' lock for read-only
	//
	// TODO: look into using journal_mode = 'WAL' (https://sqlite.org/wal.html)
	// since it is more performant and is supported by Unix and Windows systems.
	// CEV: This only helps a little and does create 2 new files
	db.setBusyTimeout(BUSY_TIMEOUT_MS);
	db.exec(
		"PRAGMA foreign_keys = 1;\n"
		"PRAGMA journal_mode = 'PERSIST';\n"
		"PRAGMA locking_mode = 'EXCLUSIVE';"
	);
	if (should_migrate_database(db)) {
		db.exec(m001_create_tables_stmt);
		db.exec(m002_create_boot_id_table);
	}
	return db;
}

static SQLite::Database open_default_database(bool readonly = false) {
	auto name = fs::path(histdb_database_path());
	if (!fs::exists(name)) {
		if (name.has_parent_path()) {
			fs::create_directories(name.remove_filename());
		}
	}
	auto sname = name.string();
	return open_database(sname, readonly);
}

// code_us_fraction encodes microsecond fraction us into char p. The behavior
// is undefined if us is greater than one second or if p is not large enough
// to store the fraction and leading '.'.
static char *code_us_fraction(int64_t us, char *p) {
	if (us == 0) {
		*p = '\0';
		return p;
	}
	int32_t i = 6;
	*p++ = '.';
	while (us % 10 == 0) {
		us /= 10;
		i--;
	}
	p[i] = '\0';
	char *end = &p[i];
	for (;;) {
		p[--i] = '0' + us % 10;
		if (i == 0) {
			break;
		}
		us /= 10;
	}
	return end;
}

static std::string format_time(const std::chrono::time_point<std::chrono::system_clock> now) {
	const auto us = std::chrono::duration_cast<std::chrono::microseconds>(
		now - std::chrono::floor<std::chrono::seconds>(now)
	);

	const std::time_t tt = std::chrono::system_clock::to_time_t(now);
	const std::tm tm = *std::localtime(&tt);

	char format[32] = "%Y-%m-%dT%H:%M:%S";
	char *frac = &format[std::strlen("%Y-%m-%dT%H:%M:%S")];

	// append microseconds to the format string => "%Y-%m-%dT%H:%M:%S.01234"
	// zone is the new end of the format string
	char *zone = code_us_fraction(us.count(), frac);

	// bytes remaining in the format string after appending microseconds
	auto bufsz = sizeof(format) - ptrdiff_t(zone - format);

	// convert zone (-0400 => -04:00) and append it to the format string
	if (std::strftime(zone, bufsz, "%z", &tm) == 5) {
		std::memmove(&zone[4], &zone[3], 3);
		zone[3] = ':';
	}

	auto dest = std::string(34, '\0');
	std::size_t n = std::strftime(&dest.at(0), dest.capacity(), format, &tm);
	if (unlikely(n == 0)) {
		throw std::runtime_error("error: failed to format time");
	}
	dest.resize(n);

	return dest;
}

static std::string get_boot_time() {
	int mib[2] = { CTL_KERN, KERN_BOOTTIME };
	struct timeval boot;
	size_t size = sizeof(boot);

	if (unlikely(sysctl(mib, 2, &boot, &size, nullptr, 0) != 0)) {
		throw ErrnoException(absl::StrCat(
			"error: ", errno, ": ", absl::NullSafeStringView(std::strerror(errno))
		));
	}

	// NB: We rely on the epoch of the system_clock being the Unix epoch,
	// which is unspecified until C++20.
	auto unix = std::chrono::seconds(boot.tv_sec) +
		std::chrono::microseconds(boot.tv_usec);
	return format_time(
		std::chrono::time_point<std::chrono::system_clock>(unix)
	);
}

// TODO:
//	* don't allow control chars
//	* pass in the raw history and trim whitespace
static void insert_history_record(SQLite::Database& db) {
	auto ts = format_time(std::chrono::system_clock::now());
	auto ppid = getppid();
	SQLite::Statement query(db, insert_history_stmt);
	query.bind(1, session_id);
	query.bind(2, history_id);
	// TODO: don't need this if it's part of the session_ids table
	query.bind(3, ppid);
	query.bind(4, status_code);
	query.bind(5, ts);
	query.bind(6, current_user);
	query.bind(7, current_wd);
	query.bind(8, raw_history);
	query.exec();
}

static int64_t new_session_id(SQLite::Database& db) {
	SQLite::Statement query(
		db, "INSERT INTO session_ids (ppid, boot_time) VALUES (?, ?);"
	);
	query.bind(1, static_cast<int32_t>(getppid()));
	query.bind(2, get_boot_time());
	query.exec();
	return db.getLastInsertRowid();
}

static int session_id_command(int argc, char * const argv[]) {
	// TODO: handle all of our exceptions
	try {
		parse_session_cmd_argments(argc, argv);
		if (print_usage) {
			session_usage();
			return EXIT_SUCCESS;
		}
		SQLite::Database db = open_default_database();
		int64_t id = new_session_id(db);
		if (print_eval) {
			std::cout << "export HISTDB_SESSION_ID=" << id << ";" << std::endl;
		} else {
			std::cout << std::to_string(id) << std::endl;
		}
		return EXIT_SUCCESS;

	} catch (const SQLite::Exception& e) {
		int ext = e.getExtendedErrorCode();
		if (ext >= 0) {
			std::cerr << "sqlite: " << e.getErrorCode() << "." << ext << ": "
				<< e.getErrorStr() << std::endl;
		} else {
			std::cerr << "sqlite: " << e.getErrorCode() << ": "
				<< e.getErrorStr() << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << std::endl;
	}
	return EXIT_FAILURE;
}

static int64_t new_boot_id(SQLite::Database& db) {
	auto ts = format_time(std::chrono::system_clock::now());
	SQLite::Statement query(
		db, "INSERT INTO boot_ids (created_at) VALUES (?);"
	);
	query.bind(1, ts);
	query.exec();
	return db.getLastInsertRowid();
}

static int boot_id_command(int argc, char * const argv[]) {
	// TODO: handle all of our exceptions
	try {
		parse_boot_id_cmd_argments(argc, argv);
		if (print_usage) {
			boot_id_usage();
			return EXIT_SUCCESS;
		}
		SQLite::Database db = open_default_database();
		int64_t id = new_boot_id(db);
		if (print_eval) {
			std::cout << "export HISTDB_SESSION_ID=" << id << ";" << std::endl;
		} else {
			std::cout << std::to_string(id) << std::endl;
		}
		return EXIT_SUCCESS;

	} catch (const SQLite::Exception& e) {
		int ext = e.getExtendedErrorCode();
		if (ext >= 0) {
			std::cerr << "sqlite: " << e.getErrorCode() << "." << ext << ": "
				<< e.getErrorStr() << std::endl;
		} else {
			std::cerr << "sqlite: " << e.getErrorCode() << ": "
				<< e.getErrorStr() << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << std::endl;
	}
	return EXIT_FAILURE;
}

static int new_boot_id_command(CLI::App *app) {
	// TODO: handle all of our exceptions
	try {
		SQLite::Database db = open_default_database();
		int64_t id = new_boot_id(db);
		if (app->get_option("--eval")->as<bool>()) {
			std::cout << "export HISTDB_BOOT_ID=" << id << ";" << std::endl;
		} else {
			std::cout << std::to_string(id) << std::endl;
		}
		return EXIT_SUCCESS;

	} catch (const SQLite::Exception& e) {
		int ext = e.getExtendedErrorCode();
		if (ext >= 0) {
			std::cerr << "sqlite: " << e.getErrorCode() << "." << ext << ": "
				<< e.getErrorStr() << std::endl;
		} else {
			std::cerr << "sqlite: " << e.getErrorCode() << ": "
				<< e.getErrorStr() << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << std::endl;
	}
	return EXIT_FAILURE;
}

// TODO: consider removing this once we restructure the code
static int dump_history_command(int argc, char * const argv[]) {
	// TODO: handle all of our exceptions
	try {
		parse_dump_history_cmd_argments(argc, argv);
		if (print_usage) {
			boot_id_usage();
			return EXIT_SUCCESS;
		}
		SQLite::Database db = open_default_database(true);

		SQLite::Statement query(
			db, "SELECT raw FROM history;"
		);

		// TODO: support line buffering.
		// Using a buffer here is ~3x faster.
		auto buf = std::string();
		buf.reserve(96 * 1024);
		while (query.executeStep()) {
			auto raw = absl::NullSafeStringView(query.getColumn(0).getText());
			if (raw.length() >= buf.capacity()) {
				if (buf.length() > 0) {
					std::cout << buf;
					buf.clear();
				}
				if (raw.length() >= 96 * 1024) {
					// Large string - write directly
					std::cout << raw << std::endl;
				}
			} else {
				buf.append(raw);
				buf.push_back('\n');
			}
		}
		if (buf.length() > 0) {
			std::cout << buf;
		}
		return EXIT_SUCCESS;

	} catch (const SQLite::Exception& e) {
		int ext = e.getExtendedErrorCode();
		if (ext >= 0) {
			std::cerr << "sqlite: " << e.getErrorCode() << "." << ext << ": "
				<< e.getErrorStr() << std::endl;
		} else {
			std::cerr << "sqlite: " << e.getErrorCode() << ": "
				<< e.getErrorStr() << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << std::endl;
	}
	return EXIT_FAILURE;
}

static int insert_command(int argc, char * const argv[]) {
	try {
		parse_insert_cmd_argments(argc, argv);
		if (print_usage) {
			insert_usage();
			return EXIT_SUCCESS;
		}
		SQLite::Database db = open_default_database();
		insert_history_record(db);
		return EXIT_SUCCESS;

	} catch (const SQLite::Exception& e) {
		int ext = e.getExtendedErrorCode();
		if (ext >= 0) {
			std::cerr << "sqlite: " << e.getErrorCode() << "." << ext << ": "
				<< e.getErrorStr() << std::endl;
		} else {
			std::cerr << "sqlite: " << e.getErrorCode() << ": "
				<< e.getErrorStr() << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << std::endl;
	}
	return EXIT_FAILURE;
}

static int db_dump_info() {
	auto dump = [](std::string name) {
		std::cout << name << ":" << std::endl;
		auto dbname = histdb_database_path();
		if (!fs::exists(dbname)) {
			std::cout << "  NONE" << std::endl;
			return;
		}
		auto db = open_default_database(true);
		auto is_prod = FORCE_USE_PROD_DATABASE || get_env_bool(HISTDB_PROD);
		std::cout << "  prod:        " << (is_prod ? "true" : "false") << std::endl;
		std::cout << "  database:    " << histdb_database_path() << std::endl;
		std::string last_ts;
		std::string last_cmd;
		int64_t rows = 0;
		try {
			SQLite::Statement query(
				db, "SELECT created_at, raw FROM history order BY id DESC LIMIT 1;"
			);
			if (!query.executeStep()) {
				return;
			}
			last_ts = query.getColumn(0).getString();
			last_cmd = query.getColumn(1).getString();
			rows = db.execAndGet("SELECT COUNT(*) FROM history;").getInt64();
		} catch (const std::exception& e) {
			last_ts = "NONE";
			last_cmd = "NONE";
		}
		std::cout << "  count:       " << rows << std::endl;
		std::cout << "  last_cmd:    `" << last_cmd << "`" << std::endl;
		std::cout << "  last_insert: " << last_ts << std::endl;
	};

	dump("TEST");
	FORCE_USE_PROD_DATABASE = true;
	dump("PROD");
	return 0;
}

// TODO: remove if not used
//
// Check for an existing file (returns error message if check fails)
class RawHistoryValidator : public CLI::Validator {
  public:
    RawHistoryValidator() : Validator("RAW_HISTORY") {
        func_ = [](std::string &raw) {
			char *end{nullptr};
			int64_t history_id = std::strtoll(raw.data(), &end, 10);
			if (history_id <= 0) {
				return absl::StrCat("non-positive: HISTORY_ID: ", history_id);
			}
			if (!end || !*end) {
				return std::string("empty argument: RAW_HISTORY");
			}
            return std::string();
        };
    }
};

using RawHistoryTuple = std::tuple<int64_t, std::string>;

RawHistoryTuple parse_raw_history(const std::string& raw) {
	char *end;
	int64_t hist_id = std::strtoll(raw.data(), &end, 10);
	// TODO: use [[annotations]]
	if (unlikely(hist_id <= 0)) {
		throw ArgumentException(absl::StrCat("non-positive: HISTORY_ID: ", hist_id));
	}
	if (unlikely(!end || !*end)) {
		throw ArgumentException(absl::StrCat("empty argument: RAW_HISTORY: ", raw));
	}
	if (!std::isspace(end[0])) {
		throw ArgumentException(absl::StrCat("non-numeric: HISTORY_ID: ", raw));
	}
	return RawHistoryTuple(hist_id, trim_ascii_space(end));
}

// static void insert_history_record(SQLite::Database& db) {
// 	auto ts = format_time(std::chrono::system_clock::now());
// 	auto ppid = getppid();
// 	SQLite::Statement query(db, insert_history_stmt);
// 	query.bind(1, session_id);
// 	query.bind(2, history_id);
// 	// TODO: don't need this if it's part of the session_ids table
// 	query.bind(3, ppid);
// 	query.bind(4, status_code);
// 	query.bind(5, ts);
// 	query.bind(6, current_user);
// 	query.bind(7, current_wd);
// 	query.bind(8, raw_history);
// 	query.exec();
// }

static int64_t new_session_id_command(CLI::App *app) {
	(void)app; // WARN: use app

	// WARN: see if the App requests prod
	//
	// Pedantically guard against writing to the real database.
	// TODO: Remove this once testing is done.
	std::string dbname;
	if (use_test_database()) {
		dbname = "test.sqlite3";
	} else {
		dbname = user_data_dir() / "histdb" / "data" / HISTDB_NAME;
	}
	SQLite::Database db = open_database(dbname);

	SQLite::Statement query(
		db, "INSERT INTO session_ids (ppid, boot_time) VALUES (?, ?);"
	);
	query.bind(1, static_cast<int32_t>(getppid()));
	query.bind(2, get_boot_time());
	query.exec();

	int64_t id = db.getLastInsertRowid();
	if (print_eval) {
		std::cout << "export HISTDB_SESSION_ID=" << id << ";" << std::endl;
	} else {
		std::cout << id << std::endl;
	}

	// TODO: don't return here - just throw on error
	return EXIT_SUCCESS;
}

static int new_insert_command(CLI::App *app) {
	try {
		// Parse first to detect errors
		auto [hist_id, raw_cmd] = parse_raw_history(
			app->get_option("history")->as<std::string>()
		);
		if (raw_cmd.length() == 0) {
			throw ArgumentException("empty raw history command");
		}

		auto session_id = app->get_option("--session")->as<int64_t>();
		if (unlikely(session_id <= 0)) {
			throw ArgumentException(absl::StrCat("non-positive session: ", session_id));
		}
		auto status_code = app->get_option("--status-code")->as<int32_t>();

		// WARN: remove
		// std::cout << "session_id: " << session_id << std::endl;
		// std::cout << "status_code: " << status_code << std::endl;

		auto pwd = must_getenv("PWD");
		auto user = must_getenv("USER");;
		auto ppid = getppid();

		// WARN: see if the App requests prod
		//
		// Pedantically guard against writing to the real database.
		// TODO: Remove this once testing is done.
		std::string dbname;
		if (use_test_database()) {
			dbname = "test.sqlite3";
		} else {
			dbname = user_data_dir() / "histdb" / "data" / HISTDB_NAME;
		}
		SQLite::Database db = open_database(dbname);

		auto ts = format_time(std::chrono::system_clock::now());
		SQLite::Statement query(db, insert_history_stmt);
		query.bind(1, session_id);
		query.bind(2, hist_id);
		// TODO: don't need this if it's part of the session_ids table
		query.bind(3, ppid);
		query.bind(4, status_code);
		query.bind(5, ts);
		query.bind(6, user);
		query.bind(7, pwd);
		query.bind(8, raw_cmd);
		query.exec();

		return EXIT_SUCCESS;

	} catch (const SQLite::Exception& e) {
		int ext = e.getExtendedErrorCode();
		if (ext >= 0) {
			std::cerr << "sqlite: " << e.getErrorCode() << "." << ext << ": "
				<< e.getErrorStr() << std::endl;
		} else {
			std::cerr << "sqlite: " << e.getErrorCode() << ": "
				<< e.getErrorStr() << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << std::endl;
	}
	return EXIT_FAILURE;
}

int root_command(int argc, char * const argv[]) {
	// App

	CLI::App app("histdb");
	app.add_option("-d,--debug", verbose, "print debug output (not supported)");

	// TODO: do we need this if we have the envname() option?
	use_prod_database = get_env_bool(HISTDB_PROD);
	app.add_flag("-p,--prod", use_prod_database, "use production database")
		->envname(std::string(HISTDB_PROD));


	auto force_dev_callback = [](int64_t){
		use_prod_database = false;
	};
	app.add_flag_function("--development", force_dev_callback,
		"force use of development/test database");

	// Session
	CLI::App *session = app.add_subcommand("session", "return a new session id");
	session->add_flag("-e,--eval", print_eval,
		"print the session id as a statment that can be evaluated by bash");

	// Info
	app.add_subcommand("info", "print information about the histdb database");

	// Insert
	CLI::App *insert = app.add_subcommand("insert", "insert a history entry");
	insert->add_option("-s,--session", "session id (required)")
		->required()
		->check(CLI::PositiveNumber);
	insert->add_option("-c,--status-code", "command status/exit code (required)")
		->required()
		->check(CLI::Number);
	// Positional "history" argument
	// TODO: validate that it matches `^\d+\s+\w+`
	insert->add_option("history", "raw history")->required();

	// Boot-id
	CLI::App *boot_id = app.add_subcommand("boot-id", "generate a new boot-id");
	boot_id->add_flag("-e,--eval", print_eval,
		"print the boot id as a statment that can be evaluated by bash");

	// TODO: add "boot-id"

	// Propagate common root flags to child commands
	for (auto *subc : app.get_subcommands(nullptr)) {
		subc->add_flag("-p,--prod", use_prod_database, "use production database")
			->envname(std::string(HISTDB_PROD));
		subc->add_flag_function("--development", force_dev_callback,
			"force use of development/test database");
	}

	// TODO: use a callback to run subcommands
	//
	// https://cliutils.github.io/CLI11/book/chapters/subcommands.html
	// https://github.com/CLIUtils/CLI11/tree/main/examples/subcom_in_files

	app.require_subcommand();

	// std::cout << "num subcommands: " << std::to_string(app.get_subcommands(nullptr).size()) << std::endl;
	CLI11_PARSE(app, argc, argv);

	if (app.got_subcommand("insert")) {
		// TODO: run insert logic
		// std::cout << "subcommand: insert" << std::endl;
		return new_insert_command(insert);
	} else if (app.got_subcommand("session")) {
		return new_session_id_command(session);
	} else if (app.got_subcommand("boot-id")) {
		return new_boot_id_command(boot_id);
	} else if (app.got_subcommand("info")) {
		return db_dump_info();
	} else {
		// WARN: is this reachable?
	}

	return 0;
}

int main(int argc, char * const argv[]) {

	// WARN WARN WARN WARN WARN WARN WARN WARN
	return root_command(argc, argv);

	if (argc <= 1) {
		root_usage(std::cerr);
		return EXIT_FAILURE;
	}

	// consume exe and command
	auto cmd = absl::NullSafeStringView(argv[1]);
	argv += 1;
	argc -= 1;

	if (cmd == "session") {
		return session_id_command(argc, argv);
	}
	if (cmd == "insert") {
		return insert_command(argc, argv);
	}
	if (cmd == "boot-id") {
		return boot_id_command(argc, argv);
	}
	if (cmd == "dump") {
		return dump_history_command(argc, argv);
	}
	// TODO: document this
	if (cmd == "info") {
		return db_dump_info();
	}
	if (cmd == "-h" || cmd == "--help") {
		root_usage();
		return 0;
	}

	// invalid arg
	if (cmd.size() > 0 && cmd.at(0) == '-') {
		// invalid flag
		std::cerr << "error: unknown flag: '" << cmd << "'\n";
	} else {
		// invalid command
		std::cerr << "error: unknown command: \"" << cmd << "\" for \"histdb\"\n";
	}
	std::cerr << "Run 'histdb --help' for usage." << std::endl;
	return EXIT_FAILURE;
}

// typedef int (*command_function)(int, char **);
//
// class Command {
// public:
// 	Command(command_function fn, std::string name) : fn(fn), name(name) {};
// 	~Command();
//
// 	bool match(const std::string& s) { return name.compare(s) == 0; };
// 	bool match(const char *s) { return name.compare(s) == 0; };
//
// 	int exec(int argc, char *argv[]) { return fn(argc, argv); };
// private:
// 	command_function fn;
// 	std::string name;
// };
//
// static Command cmds[] = {
// 	Command(nullptr, "--help"),
// };

/*
int format_timevalp(struct timeval *tv, std::string& dst) {
	struct tm *tm_info = localtime(&tv->tv_sec);
	if (unlikely(!tm_info)) {
		return -1;
	}
	int n;
	char zone[8];
	char format[32];
	char buffer[34];
	n = strftime(zone, sizeof(zone), "%z", tm_info);
	// convert: -0400 => -04:00
	if (n == 5) {
		zone[4] = zone[3];
		zone[5] = zone[4];
		zone[3] = ':';
		zone[6] = '\0';
	}

	n = snprintf(format, sizeof(format), "%%Y-%%m-%%dT%%H:%%M:%%S" ".%04d" "%s",
		tv->tv_usec, zone);
	if (unlikely(n < 0 || n >= sizeof(format))) {
		return -1;
	}

	n = strftime(buffer, sizeof(buffer), format, tm_info);
	if (unlikely(n == 0)) {
		return -1;
	}

	dst.append(buffer, n);
	return 0;
}
*/
