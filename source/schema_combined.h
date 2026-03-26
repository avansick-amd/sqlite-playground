#pragma once

#include "types.h"
#include <sqlite3.h>
#include <string>
#include <vector>

// Populate reference tables for combined schema
void populateCombinedRefs(sqlite3* db, const RefData& refs, const std::vector<KernelDispatch>& dispatches);

// Insert dispatches into combined schema. If chunk_inserts is true, use multi-row
// INSERT batches sized to SQLITE_LIMIT_VARIABLE_NUMBER; otherwise one row per statement
// (original behavior). Default is false.
void insertCombinedDispatches(sqlite3* db, const std::vector<KernelDispatch>& dispatches,
                              bool chunk_inserts = false);

// Read and process dispatches from combined schema
int64_t readCombinedDispatches(sqlite3* db);
