#include "db_helpers.h"
#include <fstream>
#include <sstream>
#include <cstdio>

sqlite3* createDatabase(const std::string& schema_file,
                       const std::string& db_name,
                       bool enable_fk,
                       bool enable_exclusive_locking) {
    sqlite3* db = nullptr;
    std::remove(db_name.c_str());
    sqlite3_open(db_name.c_str(), &db);

    std::ifstream file(schema_file);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string sql = buffer.str();
        file.close();
        sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    }

    sqlite3_exec(db, "PRAGMA synchronous = OFF;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA journal_mode = MEMORY;", nullptr, nullptr, nullptr);
    sqlite3_exec(db,
                 enable_fk ? "PRAGMA foreign_keys = ON;"
                           : "PRAGMA foreign_keys = OFF;",
                 nullptr, nullptr, nullptr);
    sqlite3_exec(db,
                 enable_exclusive_locking ? "PRAGMA locking_mode = EXCLUSIVE;"
                                           : "PRAGMA locking_mode = NORMAL;",
                 nullptr, nullptr, nullptr);

    return db;
}

bool executeSql(sqlite3* db, const std::string& sql) {
    return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool executeSqlFile(sqlite3* db, const std::string& sql_file) {
    std::ifstream file(sql_file);
    if (!file.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return executeSql(db, buffer.str());
}
