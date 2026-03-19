#pragma once

#include "types.h"
#include <sqlite3.h>
#include <string>
#include <vector>

// Populate reference tables for original schema
void populateOriginalRefs(sqlite3* db, const RefData& refs);

// Insert dispatches into original schema
void insertOriginalDispatches(sqlite3* db, const std::vector<KernelDispatch>& dispatches);

// Read and process dispatches from original schema
int64_t readOriginalDispatches(sqlite3* db);
