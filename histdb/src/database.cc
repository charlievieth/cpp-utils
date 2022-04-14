#include <cstdlib>
#include <unistd.h> // getppid

#include <histdb/database.h>
#include <histdb/exception.h>

#include <filesystem>
#include <string_view>

namespace fs = std::filesystem;

namespace histdb {

namespace {
static std::string_view safe_getenv(const char *name) {
	const char *p = std::getenv(name);
	return p ? std::string_view(p) : std::string_view();
}

static constexpr bool parse_bool(std::string_view s) noexcept {
	return static_cast<bool>(s == "1" || s == "t" || s == "T" || s == "true" ||
		s == "True" || s == "TRUE");
}

static fs::path user_data_dir() {
	if (auto s = safe_getenv("XDG_DATA_HOME"); !s.empty()) {
		return s;
	}
	if (auto s = safe_getenv("HOME"); !s.empty()) {
		return fs::path(s) / ".local" / "share";
	}
	throw histdb::ArgumentException("neither $XDG_DATA_HOME nor $HOME are defined");
}

static fs::path histdb_database_path() {
	// Pedantically guard against writing to the real database.
	// TODO: Remove this once testing is done.
	if (!parse_bool(safe_getenv(HISTDB_ENV_KEY.data()))) {
		return "test.sqlite3";
	}
	return user_data_dir() / "histdb" / "data" / HISTDB_NAME;
}
} // namespace

DatabasePtr OpenDatabase(std::string& filename, bool readonly) {
	int flags = readonly ?
		SQLite::OPEN_READONLY :
		SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE;
	auto db = std::make_unique<SQLite::Database>(filename, flags);
	db->exec(
		"PRAGMA foreign_keys = 1;\n"
		"PRAGMA journal_mode = 'PERSIST';\n"
		"PRAGMA locking_mode = 'EXCLUSIVE';"
	);
	return db;
}

DatabasePtr OpenDefaultDatabase(bool readonly) {
	auto name = fs::path(histdb_database_path());
	if (!fs::exists(name)) {
		fs::create_directories(fs::path(name).remove_filename());
	}
	auto s = name.string();
	return OpenDatabase(s, readonly);
}

Database::Database(DatabasePtr db) {
	db_ = std::move(db);
}

Database::Database(std::string& filename, bool readonly) :
	Database(OpenDatabase(filename, readonly))
{
}

int64_t Database::NewSessionID() {
	// WARN WARN WARN: need to implement boot time (or it's replacement)
	throw std::runtime_error("NOT IMPLEMENTED!");
	SQLite::Statement query(
		*db_, "INSERT INTO session_ids (ppid, boot_time) VALUES (?, ?);"
	);
	query.bind(1, static_cast<int32_t>(getppid()));
	// query.bind(2, get_boot_time());
	query.exec();
	return 1;
}

} // namespace histdb
