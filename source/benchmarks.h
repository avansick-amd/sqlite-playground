#pragma once

#include "types.h"
#include <vector>

// Global data shared across all benchmarks
extern std::vector<KernelDispatch> g_dispatches;
extern RefData g_refs;
extern bool g_data_loaded;
