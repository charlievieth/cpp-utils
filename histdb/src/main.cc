#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cerrno>
#include <ctime>

#include <filesystem>
namespace fs = std::filesystem;

#include <assert.h>
#include <sys/sysctl.h>
#include <sys/time.h> // gettimeofday, timeval
#include <getopt.h>

#include <SQLiteCpp/Database.h>

// TODO: use or remove
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// Options
////////////////////////////////////////////////////////////////////////////////

static const char *const create_tables_stmt =R"""(
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

static const char *const insert_history_stmt =R"""(
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

static const char *const root_usage_msg =R"""(histdb: shell history tool

Usage:
  histdb [command]

Available Commands:
  session: return a new session id
  insert:  insert a history entry

Use "histdb [command] --help" for more information about a command.
)""";

static const char *const insert_help_msg =R"""(insert a shell command into the histdb

Usage:
  histdb insert [flags] [history_id] [raw_command]

Flags:
    -d, --debug          print debug output (not supported)
    -s, --session        session id (required)
    -c, --status-code    command status/exit code (required)
        --dry-run        perform a "dry run" (not supported)
    -h, --help           help for insert
)""";

static const char *const session_help_msg =R"""(initiate a new histdb session

Usage:
  histdb session [flags]

Flags:
    -e, --eval    print the session id as a statment that can be evaluated by bash
    -h, --help    help for insert
)""";

// insert command options

static bool debug = false;
static bool print_usage = false;
static bool dry_run = false;
static int status_code = 0;
static long long session = 0;
static long long history_id = 0;
static std::string raw_history;
std::string current_wd;
std::string current_user;

static int opt_val;
static struct option insert_cmd_opts[] = {
	{"debug",        no_argument,       NULL, 'd'},
	{"help",         no_argument,       NULL, 'h'},
	{"session",      required_argument, NULL, 's'},
	{"status-code",  required_argument, NULL, 'c'},
	{"dry-run",      no_argument,       &opt_val, 1}, // TODO: remove if not used
	{NULL, 0, NULL, 0}, // zero pad end
};

// session command options

static bool print_eval = false;
static struct option session_cmd_opts[] = {
	{"help",         no_argument,       NULL, 'h'},
	{"eval",         no_argument,       NULL, 'e'}, // print evalable output
	{NULL, 0, NULL, 0}, // zero pad end
};

// expections (TODO: move to separate file)

class ErrnoException : public std::runtime_error {
public:
	ErrnoException(const std::string& message) : std::runtime_error(message) {}
	// ErrnoException(const int _errno) : std::runtime_error(message) {}
};

class ArgumentException : public std::invalid_argument {
public:
	ArgumentException(const std::string& message) : std::invalid_argument(message) {}
};

// usage helpers

static void root_usage(std::ostream& out = std::cout) {
	out << root_usage_msg;
	out.flush();
}

static void session_usage(std::ostream& out = std::cout) {
	out << session_help_msg;
	out.flush();
}

static void insert_usage(std::ostream& out = std::cout) {
	out << insert_help_msg;
	out.flush();
}

// parse helpers

static std::string getenv_check(const char *env_var) {
	char *tmp = std::getenv(env_var);
	if (!tmp || !*tmp) {
		throw ArgumentException("empty environment variable: " + std::string(env_var));
	}
	return std::string(tmp);
}

static bool is_ascii_space(unsigned char c) {
	switch (c) {
	case '\t':
	case '\n':
	case '\v':
	case '\f':
	case '\r':
	case ' ':
		return true;
	}
	return false;
}

static std::string trim_ansi_space(char *s) {
	unsigned char *p = (unsigned char *)s;
	while (p && is_ascii_space(*p)) {
		p++;
	};
	if (p) {
		s = (char*)p;
		size_t n = std::strlen(s);
		p = (unsigned char *)&s[n - 1];
		while (p && is_ascii_space(*p)) {
			*p = '\0';
			p--;
		};
	}
	return s;
}

// command line parsers

static void parse_insert_cmd_argments(int argc, char *argv[]) {
	int ch;
	int opt_index = 0;
	bool status_set = false;
	bool session_set = false;
	while ((ch = getopt_long(argc, argv, "dhs:c:", insert_cmd_opts, &opt_index)) != -1) {
		switch (ch) {
		case 'd':
			// WARN: not supported
			std::cerr << "--debug is not supported";
			debug = true;
			break;
		case 'h':
			print_usage = true;
			break;
		case 's':
			session = std::strtoll(optarg, NULL, 10);
			if (session <= 0) {
				throw ArgumentException("non-positive session: " + std::to_string(session));
			}
			session_set = true;
			break;
		case 'c':
			// TODO: check for any exceptions
			status_code = std::stoi(optarg, NULL, 10);
			status_set = true;
			break;
		case 0:
			switch (opt_val) {
			case 1:
				dry_run = true;
				std::cerr << "--dry-run is not supported";
				break;
			default:
				throw ArgumentException("invalid argument: " + std::to_string(opt_val));
				break;
			}
		default:
			throw ArgumentException("invalid argument: " + std::to_string(ch));
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
	if (argc != 2) {
		throw ArgumentException("expected 2 arguments ([HISTORY_ID] [RAW_COMMAND]) got: " +
			std::to_string(argc));
	}

	history_id = std::strtoll(argv[0], NULL, 10);
	if (history_id <= 0) {
		throw ArgumentException("non-positive: HISTORY_ID: " + std::to_string(history_id));
	}
	if (!argv[1] || !*argv[1]) {
		throw ArgumentException("empty argument: RAW_HISTORY");
	}
	raw_history = trim_ansi_space(argv[1]);

	current_wd = getenv_check("PWD");
	current_user = getenv_check("USER");;
}

static void parse_session_cmd_argments(int argc, char *argv[]) {
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
			throw ArgumentException("invalid argument: " + std::to_string(ch));
			break;
		}
	}
	if (print_usage) {
		return;
	}
	argc -= optind;
	argv += optind;
	if (argc != 0) {
		auto msg = std::string("unexpected arguments: ");
		msg.append("\"");
		msg.append(argv[0]);
		msg.append("\"");
		for (int i = 1; i < argc; i++) {
			msg.append(", \"");
			msg.append(argv[i]);
			msg.append("\"");
		}
		throw ArgumentException(msg);
	}
}

////////////////////////////////////////////////////////////////////////////////

// #define HISTDB_NAME "test_histdb.sqlite3"
// HERE HERE HERE

bool get_bool_env(const char *name) {
	char *s = std::getenv(name);
	if (s) {
		switch (std::strlen(s)) {
		case 1:
			switch (*s) {
			case '1':
			case 't':
			case 'T':
				return true;
			}
		case 4:
			auto x = std::string(s);
			return x == "true" || x == "True" || x == "TRUE";
			return std::strcmp(s, "true") == 0 ||
				std::strcmp(s, "TRUE") == 0 ||
				std::strcmp(s, "True") == 0;
		}
	}
	return false;
}

fs::path user_data_dir() {
	char *s = std::getenv("XDG_DATA_HOME");
	if (s && *s) {
		return s;
	}
	s = std::getenv("HOME");
	if (s && *s) {
		return fs::path(s) / ".local" / "share";
	}
	throw ArgumentException("neither $XDG_DATA_HOME nor $HOME are defined");
}

static SQLite::Database open_database(const char *filename, bool readonly = false) {
	int flags = readonly ?
		SQLite::OPEN_READONLY :
		SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE;
	SQLite::Database db = SQLite::Database(filename, flags);
	if (!db.tableExists("history")) {
		db.exec(create_tables_stmt);
	}
	db.exec(
		"PRAGMA foreign_keys = 1;\n"
		"PRAGMA journal_mode = 'PERSIST';\n"
		"PRAGMA locking_mode = 'EXCLUSIVE';"
	);
	return db;
}

static void check_errno(int ret, int exp = 0) {
	if (unlikely(ret != exp)) {
		char *str = std::strerror(errno);
		std::string msg;
		msg.reserve(std::strlen(str) + 14); // 14 == strlen("error: 12345: ")
		msg.append("error: ");
		msg.append(std::to_string(errno));
		msg.append(": ");
		msg.append(str);
		throw ErrnoException(msg);
	}
}

static int format_timeval(struct timeval *tv, std::string& dst) {
	struct tm *tm_info = std::localtime(&tv->tv_sec);
	assert(tm_info);

	int n;
	char zone[8];
	char format[32];
	char buffer[34];
	// convert: -0400 => -04:00
	if (std::strftime(zone, sizeof(zone), "%z", tm_info) == 5) {
		std::memmove(&zone[4], &zone[3], 3);
		zone[3] = ':';
	}

	n = snprintf(format, sizeof(format), "%%Y-%%m-%%dT%%H:%%M:%%S" ".%04d" "%s",
		tv->tv_usec, zone);
	assert((size_t)n <= sizeof(format));

	n = std::strftime(buffer, sizeof(buffer), format, tm_info);
	assert(n != 0);

	dst.append(buffer, n);
	return 0;
}

static std::string get_current_timestamp() {
	struct timeval tv;
	check_errno(gettimeofday(&tv, NULL));
	std::string s;
	format_timeval(&tv, s);
	return s;
}

static std::string get_boot_time() {
	struct timeval boot;
	int mib[2] = { CTL_KERN, KERN_BOOTTIME };
	size_t size = sizeof(boot);

	check_errno(sysctl(mib, 2, &boot, &size, NULL, 0));

	std::string s;
	format_timeval(&boot, s);
	return s;
}

// TODO: pass in the raw history and trim whitespace
static void insert_history_record(SQLite::Database& db) {
	auto ts = get_current_timestamp();
	auto ppid = getppid();
	SQLite::Statement query(db, insert_history_stmt);
	query.bind(1, session);
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
	query.bind(1, (int)getppid());
	query.bind(2, get_boot_time());
	query.exec();
	return db.getLastInsertRowid();
}

static int session_id_command(int argc, char *argv[]) {
	// TODO: handle all of our exceptions
	try {
		parse_session_cmd_argments(argc, argv);
		if (print_usage) {
			session_usage();
			return EXIT_SUCCESS;
		}
		SQLite::Database db = open_database("test.sqlite3");
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

static int insert_command(int argc, char *argv[]) {
	try {
		parse_insert_cmd_argments(argc, argv);
		if (print_usage) {
			insert_usage();
			return EXIT_SUCCESS;
		}
		SQLite::Database db = open_database("test.sqlite3");
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

int main(int argc, char *argv[]) {
	if (argc == 1) {
		root_usage(std::cerr);
		return EXIT_FAILURE;
	}

	if (std::strcmp(argv[1], "session") == 0) {
		return session_id_command(argc - 1, &argv[1]);
	}
	if (std::strcmp(argv[1], "insert") == 0) {
		return insert_command(argc - 1, &argv[1]);
	}
	if (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0) {
		root_usage();
		return EXIT_FAILURE;
	}

	// invalid arg
	if (*argv[1] == '-') {
		// invalid flag
		std::cerr << "Error: unknown flag: '" << argv[1] << "'\n";
	} else {
		// invalid command
		std::cerr << "Error: unknown command: \"" << argv[1] << "\" for \"histdb\"\n";
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
