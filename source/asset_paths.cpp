#include "asset_paths.h"
#include <array>
#include <cstdlib>
#include <filesystem>
#if defined(__linux__)
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static fs::path computeAssetsRoot() {
    if (const char* env = std::getenv("ROCPD_BENCH_RESOURCE_DIR")) {
        return fs::path(env);
    }
#if defined(__linux__)
    std::array<char, 4096> buf{};
    const ssize_t n = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
    if (n > 0) {
        buf[static_cast<size_t>(n)] = '\0';
        return fs::path(buf.data()).parent_path();
    }
#endif
    return fs::current_path();
}

std::string benchmarkAsset(std::string_view relative_path) {
    static const fs::path root = computeAssetsRoot();
    return (root / relative_path).string();
}
