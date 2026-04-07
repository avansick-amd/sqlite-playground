#pragma once

#include "types.h"
#include <sqlite3.h>
#include <string>
#include <vector>

// Populate reference tables for original schema
void populateOriginalRefs(sqlite3* db, const RefData& refs);

// Insert dispatches into original schema. If chunk_inserts is true, use multi-row
// INSERT batches sized to SQLITE_LIMIT_VARIABLE_NUMBER; otherwise one row per statement
// (original behavior). Default is false.
void insertOriginalDispatches(sqlite3* db, const std::vector<KernelDispatch>& dispatches,
                              bool chunk_inserts = false);

// Read and process dispatches from original schema
int64_t readOriginalDispatches(sqlite3* db);

// Read path for a real ROCpd capture file (tables `rocpd_*_<GUID>`), same JOIN shape as
// readOriginalDispatches but with GUID-suffixed names.
int64_t readRocpdSourceDispatchJoin(sqlite3* db);

// CREATE INDEX on `rocpd_kernel_dispatch_<GUID>` (supplemental perf indexes, RW connection).
void createRocpdSourceSchemaPerformanceIndexes(sqlite3* db);

// Row count in `rocpd_kernel_dispatch_<GUID>` (for items_per_second counters).
int64_t countRocpdSourceDispatches(sqlite3* db);
