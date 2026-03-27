#pragma once

#include "data_loader.h"
#include <vector>

// Global data shared across all benchmarks (reloaded when DispatchExtractOrder changes).
extern std::vector<KernelDispatch> g_dispatches;
extern RefData g_refs;
extern bool g_data_loaded;
