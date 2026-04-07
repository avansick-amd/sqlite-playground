#pragma once

#include <string>
#include <string_view>

// Directory containing copied SQL files and benchmark_data/ (typically the CMake build dir).
// Override with env ROCPD_BENCH_RESOURCE_DIR if needed.
std::string benchmarkAsset(std::string_view relative_path);
