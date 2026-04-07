#include "benchmarks.h"
#include "asset_paths.h"
#include "data_loader.h"
#include "db_helpers.h"
#include "schema_original.h"
#include "schema_combined.h"
#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <string>

// Global data storage
std::vector<KernelDispatch> g_dispatches;
RefData g_refs;
bool g_data_loaded = false;

void ensureBenchmarkDataLoaded() {
    if (!g_data_loaded) {
        extractRealData(benchmarkAsset("benchmark_data/rocpd-7389.db"), g_dispatches,
                        g_refs);
        g_data_loaded = true;
    }
}

namespace {

template <bool ChunkedInserts, bool NoFKExclAccess, bool SupplementalIndexes, bool DeferredIndexes>
void BM_Write_Original(benchmark::State& state) {
    ensureBenchmarkDataLoaded();
    const std::string kDb = std::string(state.name()) + ".db";
    for (auto _ : state) {
        sqlite3* db = createDatabase(benchmarkAsset("rocpd_tables.sql"), kDb,
                                     !NoFKExclAccess, NoFKExclAccess);
        if constexpr (SupplementalIndexes && !DeferredIndexes) {
            executeSqlFile(db, benchmarkAsset("rocpd_indexes.sql"));
        }
        populateOriginalRefs(db, g_refs);
        insertOriginalDispatches(db, g_dispatches, ChunkedInserts);
        if constexpr (SupplementalIndexes && DeferredIndexes) {
            executeSqlFile(db, benchmarkAsset("rocpd_indexes.sql"));
        }
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(g_dispatches.size()));
    std::remove(kDb.c_str());
}

template <bool ChunkedInserts, bool NoFKExclAccess, bool SupplementalIndexes, bool DeferredIndexes>
void BM_Write_Combined(benchmark::State& state) {
    ensureBenchmarkDataLoaded();
    const std::string kDb = std::string(state.name()) + ".db";
    for (auto _ : state) {
        sqlite3* db =
            createDatabase(benchmarkAsset("rocpd_tables_combined.sql"), kDb,
                           !NoFKExclAccess, NoFKExclAccess);
        if constexpr (SupplementalIndexes && !DeferredIndexes) {
            executeSqlFile(db, benchmarkAsset("rocpd_combined_indexes.sql"));
        }
        populateCombinedRefs(db, g_refs, g_dispatches);
        insertCombinedDispatches(db, g_dispatches, ChunkedInserts);
        if constexpr (SupplementalIndexes && DeferredIndexes) {
            executeSqlFile(db, benchmarkAsset("rocpd_combined_indexes.sql"));
        }
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(g_dispatches.size()));
    std::remove(kDb.c_str());
}

template <bool SupplementalIndexes, bool DeferredIndexes>
void BM_Read_Original(benchmark::State& state) {
    ensureBenchmarkDataLoaded();
    const std::string kDb = std::string(state.name()) + ".db";
    sqlite3* db_load =
        createDatabase(benchmarkAsset("rocpd_tables.sql"), kDb, true, false);
    if constexpr (SupplementalIndexes && !DeferredIndexes) {
        executeSqlFile(db_load, benchmarkAsset("rocpd_indexes.sql"));
    }
    populateOriginalRefs(db_load, g_refs);
    insertOriginalDispatches(db_load, g_dispatches, false);
    if constexpr (SupplementalIndexes && DeferredIndexes) {
        executeSqlFile(db_load, benchmarkAsset("rocpd_indexes.sql"));
    }
    sqlite3_close(db_load);

    sqlite3* db_read = nullptr;
    sqlite3_open(kDb.c_str(), &db_read);
    applyReadBenchmarkPragmas(db_read);

    for (auto _ : state) {
        benchmark::DoNotOptimize(readOriginalDispatches(db_read));
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(g_dispatches.size()));
    sqlite3_close(db_read);
    std::remove(kDb.c_str());
}

template <bool SupplementalIndexes, bool DeferredIndexes>
void BM_Read_Combined(benchmark::State& state) {
    ensureBenchmarkDataLoaded();
    const std::string kDb = std::string(state.name()) + ".db";
    sqlite3* db_load =
        createDatabase(benchmarkAsset("rocpd_tables_combined.sql"), kDb, true, false);
    if constexpr (SupplementalIndexes && !DeferredIndexes) {
        executeSqlFile(db_load, benchmarkAsset("rocpd_combined_indexes.sql"));
    }
    populateCombinedRefs(db_load, g_refs, g_dispatches);
    insertCombinedDispatches(db_load, g_dispatches, false);
    if constexpr (SupplementalIndexes && DeferredIndexes) {
        executeSqlFile(db_load, benchmarkAsset("rocpd_combined_indexes.sql"));
    }
    sqlite3_close(db_load);

    sqlite3* db_read = nullptr;
    sqlite3_open(kDb.c_str(), &db_read);
    applyReadBenchmarkPragmas(db_read);

    for (auto _ : state) {
        benchmark::DoNotOptimize(readCombinedDispatches(db_read));
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(g_dispatches.size()));
    sqlite3_close(db_read);
    std::remove(kDb.c_str());
}

// Control: read the shipped ROCpd capture in place (GUID-suffixed tables), no synthetic DB.
void BM_Read_ROCpdSourceFile(benchmark::State& state) {
    const std::string src = benchmarkAsset("benchmark_data/rocpd-7389.db");
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(src.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        state.SkipWithError("failed to open benchmark_data/rocpd-7389.db");
        return;
    }
    applyReadBenchmarkPragmas(db);
    const int64_t n_disp = countRocpdSourceDispatches(db);
    if (n_disp <= 0) {
        sqlite3_close(db);
        state.SkipWithError("no rows in rocpd_kernel_dispatch_<GUID> (wrong schema?)");
        return;
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(readRocpdSourceDispatchJoin(db));
    }
    state.SetItemsProcessed(state.iterations() * n_disp);
    sqlite3_close(db);
}

// Copy source file, add supplemental indexes on the real dispatch table, then read (timed).
void BM_Read_ROCpdSourceFile_IndexedCopy(benchmark::State& state) {
    namespace fs = std::filesystem;
    const std::string src = benchmarkAsset("benchmark_data/rocpd-7389.db");
    const std::string work = std::string(state.name()) + ".db";
    std::error_code ec;
    fs::remove(work, ec);
    fs::copy_file(src, work, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        state.SkipWithError("failed to copy source db for indexed benchmark");
        return;
    }

    sqlite3* db_rw = nullptr;
    sqlite3_open(work.c_str(), &db_rw);
    createRocpdSourceSchemaPerformanceIndexes(db_rw);
    sqlite3_close(db_rw);

    sqlite3* db = nullptr;
    if (sqlite3_open_v2(work.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        fs::remove(work, ec);
        state.SkipWithError("failed to open indexed copy");
        return;
    }
    applyReadBenchmarkPragmas(db);
    const int64_t n_disp = countRocpdSourceDispatches(db);

    for (auto _ : state) {
        benchmark::DoNotOptimize(readRocpdSourceDispatchJoin(db));
    }
    state.SetItemsProcessed(state.iterations() * n_disp);
    sqlite3_close(db);
    fs::remove(work, ec);
}

BENCHMARK_TEMPLATE(BM_Write_Original, false, false, false, false)
    ->Name("BM_Write_Original")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Original, true, false, false, false)
    ->Name("BM_Write_Original_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Read_Original, false, false)
    ->Name("BM_Read_Original")
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Read_ROCpdSourceFile)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Read_ROCpdSourceFile_IndexedCopy)->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Write_Original, false, true, false, false)
    ->Name("BM_Write_Original_NoFKExclAccess")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Original, true, true, false, false)
    ->Name("BM_Write_Original_NoFKExclAccess_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);

// Supplemental indexes before data
BENCHMARK_TEMPLATE(BM_Write_Original, false, false, true, false)
    ->Name("BM_Write_Original_Indexed_Initial")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Original, true, false, true, false)
    ->Name("BM_Write_Original_Indexed_Initial_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Read_Original, true, false)
    ->Name("BM_Read_Original_Indexed_Initial")
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Write_Original, false, true, true, false)
    ->Name("BM_Write_Original_Indexed_Initial_NoFKExclAccess")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Original, true, true, true, false)
    ->Name("BM_Write_Original_Indexed_Initial_NoFKExclAccess_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);

// Supplemental indexes after insert
BENCHMARK_TEMPLATE(BM_Write_Original, false, false, true, true)
    ->Name("BM_Write_Original_Indexed_Deferred")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Original, true, false, true, true)
    ->Name("BM_Write_Original_Indexed_Deferred_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Read_Original, true, true)
    ->Name("BM_Read_Original_Indexed_Deferred")
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Write_Original, false, true, true, true)
    ->Name("BM_Write_Original_Indexed_Deferred_NoFKExclAccess")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Original, true, true, true, true)
    ->Name("BM_Write_Original_Indexed_Deferred_NoFKExclAccess_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);

// --- Combined schema ---

BENCHMARK_TEMPLATE(BM_Write_Combined, false, false, false, false)
    ->Name("BM_Write_Combined")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Combined, true, false, false, false)
    ->Name("BM_Write_Combined_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Read_Combined, false, false)
    ->Name("BM_Read_Combined")
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Write_Combined, false, true, false, false)
    ->Name("BM_Write_Combined_NoFKExclAccess")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Combined, true, true, false, false)
    ->Name("BM_Write_Combined_NoFKExclAccess_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Write_Combined, false, false, true, false)
    ->Name("BM_Write_Combined_Indexed_Initial")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Combined, true, false, true, false)
    ->Name("BM_Write_Combined_Indexed_Initial_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Read_Combined, true, false)
    ->Name("BM_Read_Combined_Indexed_Initial")
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Write_Combined, false, true, true, false)
    ->Name("BM_Write_Combined_Indexed_Initial_NoFKExclAccess")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Combined, true, true, true, false)
    ->Name("BM_Write_Combined_Indexed_Initial_NoFKExclAccess_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Write_Combined, false, false, true, true)
    ->Name("BM_Write_Combined_Indexed_Deferred")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Combined, true, false, true, true)
    ->Name("BM_Write_Combined_Indexed_Deferred_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Read_Combined, true, true)
    ->Name("BM_Read_Combined_Indexed_Deferred")
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE(BM_Write_Combined, false, true, true, true)
    ->Name("BM_Write_Combined_Indexed_Deferred_NoFKExclAccess")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_Write_Combined, true, true, true, true)
    ->Name("BM_Write_Combined_Indexed_Deferred_NoFKExclAccess_ChunkedInserts")
    ->Unit(benchmark::kMillisecond);

}  // namespace
