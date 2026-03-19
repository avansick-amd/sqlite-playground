#pragma once

#include "types.h"
#include <string>
#include <vector>

// Load real data from ROCpd database
void extractRealData(const std::string& db_path,
                     std::vector<KernelDispatch>& dispatches,
                     RefData& refs);
