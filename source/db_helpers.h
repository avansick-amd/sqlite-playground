#pragma once

#include <sqlite3.h>
#include <string>

// Create and configure a database with given schema
sqlite3* createDatabase(const std::string& schema_file,
                       const std::string& db_name,
                       bool enable_fk = true,
                       bool enable_exclusive_locking = false);

// Run arbitrary SQL on an open database (multiple statements allowed).
// Returns true if sqlite3_exec completed without error.
bool executeSql(sqlite3* db, const std::string& sql);

// Read SQL from a file and execute it (e.g. deferred CREATE INDEX after bulk load).
bool executeSqlFile(sqlite3* db, const std::string& sql_file);

// Connection settings used when timing read-only benchmarks.
void applyReadBenchmarkPragmas(sqlite3* db);
