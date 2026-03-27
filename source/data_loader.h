#pragma once

#include "types.h"
#include <string>
#include <vector>

// How to order rows when reading rocpd_kernel_dispatch_* (affects benchmark insert order).
enum class DispatchExtractOrder {
    ByStart,    // ORDER BY start — time order
    ById,       // ORDER BY id — profiler insert / PK order
    Unordered   // no ORDER BY — engine-defined scan order
};

// Load real data from ROCpd database. Clears and refills `dispatches` and `refs`.
void extractRealData(const std::string& db_path,
                     std::vector<KernelDispatch>& dispatches,
                     RefData& refs,
                     DispatchExtractOrder dispatch_order = DispatchExtractOrder::ById);
