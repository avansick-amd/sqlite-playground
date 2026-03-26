#include "benchmarks.h"
#include "data_loader.h"
#include "db_helpers.h"
#include "schema_original.h"
#include "schema_combined.h"
#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdio>
#include <string>

// Global data storage
std::vector<KernelDispatch> g_dispatches;
RefData g_refs;
bool g_data_loaded = false;

namespace {

void ensureDataLoaded() {
    if (!g_data_loaded) {
        extractRealData("benchmark_data/rocpd-7389.db", g_dispatches, g_refs);
        g_data_loaded = true;
    }
}

void applyReadConnectionPragmas(sqlite3* db) {
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA locking_mode = NORMAL;", nullptr, nullptr, nullptr);
}

template <bool ChunkedInserts, bool NoFKExclAccess, bool SupplementalIndexes, bool DeferredIndexes>
void BM_Write_Original(benchmark::State& state) {
    ensureDataLoaded();
    const std::string kDb = std::string(state.name()) + ".db";
    for (auto _ : state) {
        sqlite3* db = createDatabase("rocpd_tables.sql", kDb, !NoFKExclAccess,
                                     NoFKExclAccess);
        if constexpr (SupplementalIndexes && !DeferredIndexes) {
            executeSqlFile(db, "rocpd_indexes.sql");
        }
        populateOriginalRefs(db, g_refs);
        insertOriginalDispatches(db, g_dispatches, ChunkedInserts);
        if constexpr (SupplementalIndexes && DeferredIndexes) {
            executeSqlFile(db, "rocpd_indexes.sql");
        }
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(g_dispatches.size()));
    std::remove(kDb.c_str());
}

template <bool ChunkedInserts, bool NoFKExclAccess, bool SupplementalIndexes, bool DeferredIndexes>
void BM_Write_Combined(benchmark::State& state) {
    ensureDataLoaded();
    const std::string kDb = std::string(state.name()) + ".db";
    for (auto _ : state) {
        sqlite3* db =
            createDatabase("rocpd_tables_combined.sql", kDb, !NoFKExclAccess,
                           NoFKExclAccess);
        if constexpr (SupplementalIndexes && !DeferredIndexes) {
            executeSqlFile(db, "rocpd_combined_indexes.sql");
        }
        populateCombinedRefs(db, g_refs, g_dispatches);
        insertCombinedDispatches(db, g_dispatches, ChunkedInserts);
        if constexpr (SupplementalIndexes && DeferredIndexes) {
            executeSqlFile(db, "rocpd_combined_indexes.sql");
        }
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(g_dispatches.size()));
    std::remove(kDb.c_str());
}

template <bool SupplementalIndexes, bool DeferredIndexes>
void BM_Read_Original(benchmark::State& state) {
    ensureDataLoaded();
    const std::string kDb = std::string(state.name()) + ".db";
    sqlite3* db_load = createDatabase("rocpd_tables.sql", kDb, true, false);
    if constexpr (SupplementalIndexes && !DeferredIndexes) {
        executeSqlFile(db_load, "rocpd_indexes.sql");
    }
    populateOriginalRefs(db_load, g_refs);
    insertOriginalDispatches(db_load, g_dispatches, false);
    if constexpr (SupplementalIndexes && DeferredIndexes) {
        executeSqlFile(db_load, "rocpd_indexes.sql");
    }
    sqlite3_close(db_load);

    sqlite3* db_read = nullptr;
    sqlite3_open(kDb.c_str(), &db_read);
    applyReadConnectionPragmas(db_read);

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
    ensureDataLoaded();
    const std::string kDb = std::string(state.name()) + ".db";
    sqlite3* db_load =
        createDatabase("rocpd_tables_combined.sql", kDb, true, false);
    if constexpr (SupplementalIndexes && !DeferredIndexes) {
        executeSqlFile(db_load, "rocpd_combined_indexes.sql");
    }
    populateCombinedRefs(db_load, g_refs, g_dispatches);
    insertCombinedDispatches(db_load, g_dispatches, false);
    if constexpr (SupplementalIndexes && DeferredIndexes) {
        executeSqlFile(db_load, "rocpd_combined_indexes.sql");
    }
    sqlite3_close(db_load);

    sqlite3* db_read = nullptr;
    sqlite3_open(kDb.c_str(), &db_read);
    applyReadConnectionPragmas(db_read);

    for (auto _ : state) {
        benchmark::DoNotOptimize(readCombinedDispatches(db_read));
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(g_dispatches.size()));
    sqlite3_close(db_read);
    std::remove(kDb.c_str());
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
