#pragma once

#include "types.h"
#include <sqlite3.h>
#include <string>
#include <vector>

// Populate reference tables for combined schema
void populateCombinedRefs(sqlite3* db, const RefData& refs, const std::vector<KernelDispatch>& dispatches);

// Insert dispatches into combined schema
void insertCombinedDispatches(sqlite3* db, const std::vector<KernelDispatch>& dispatches);

// Read and process dispatches from combined schema
int64_t readCombinedDispatches(sqlite3* db);
