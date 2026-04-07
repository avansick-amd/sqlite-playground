// SQLite Schema Benchmark - Main Entry Point
// Benchmarks different database schema designs for ROCpd profiling data

#include "cli_read_campaign.h"
#include <benchmark/benchmark.h>
#include <cstring>

int main(int argc, char** argv) {
    if (argc >= 2) {
        if (std::strcmp(argv[1], "prepare-read-dbs") == 0) {
            return runPrepareReadDbs(argc, argv);
        }
        if (std::strcmp(argv[1], "read") == 0) {
            return runReadOnceCli(argc, argv);
        }
    }

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
