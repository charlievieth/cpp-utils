#pragma once

#include <string>
#include <SQLiteCpp/Database.h>

namespace histdb {

constexpr std::string_view HISTDB_NAME = "histdb.sqlite3";
constexpr std::string_view HISTDB_ENV_KEY = "HISTDB_PROD";

using DatabasePtr = std::unique_ptr<SQLite::Database>;

class Database {
	Database(std::unique_ptr<SQLite::Database> db);

	Database(std::string& filename, bool readonly = false);

	int64_t NewSessionID();

private:
	// TODO: can we use "move" here ???
	//
	// SQLite::Database sqliteDB = nullptr;
	// std::unique_ptr<SQLite::Database> sqliteDB;
	DatabasePtr db_;
	// SQLite::Database mDB;
};

DatabasePtr OpenDatabase(std::string& filename, bool readonly = false);
DatabasePtr OpenDefaultDatabase(bool readonly = false);

} // namespace histdb
