#pragma once

#include <sqlite3.h>
#include <string>

// Create and configure a database with given schema
sqlite3* createDatabase(const std::string& schema_file,
                       const std::string& db_name,
                       bool enable_fk = true);
