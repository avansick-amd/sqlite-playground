#include "benchmarks.h"
#include "db_helpers.h"
#include "schema_original.h"
#include "schema_combined.h"
#include <benchmark/benchmark.h>
#include <cstdio>
#include <string>

std::vector<KernelDispatch> g_dispatches;
RefData g_refs;
bool g_data_loaded = false;

namespace {

// Initial value only matters before the first benchmark; registration order below is
// grouped by extract order so ensureDataLoaded reloads at most once per Order group.
DispatchExtractOrder g_loaded_order = DispatchExtractOrder::ById;

void ensureDataLoaded(DispatchExtractOrder order) {
    if (g_data_loaded && g_loaded_order == order) {
        return;
    }
    extractRealData("benchmark_data/rocpd-7389.db", g_dispatches, g_refs, order);
    g_loaded_order = order;
    g_data_loaded = true;
}

void applyReadConnectionPragmas(sqlite3* db) {
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA locking_mode = NORMAL;", nullptr, nullptr, nullptr);
}

// Deferred CREATE INDEX can leave freelist / file bloat; VACUUM rewrites the DB so read
// performance is more comparable to the indexed-initial path.
void vacuumAfterDeferredIndexes(sqlite3* db) {
    executeSql(db, "VACUUM;");
}

template <DispatchExtractOrder Order, bool ChunkedInserts, bool NoFKExclAccess,
          bool SupplementalIndexes, bool DeferredIndexes>
void BM_Write_Original(benchmark::State& state) {
    ensureDataLoaded(Order);
    const std::string kDb = std::string(state.name()) + ".db";
    for (auto _ : state) {
        sqlite3* db =
            createDatabase("rocpd_tables.sql", kDb, !NoFKExclAccess, NoFKExclAccess);
        if constexpr (SupplementalIndexes && !DeferredIndexes) {
            executeSqlFile(db, "rocpd_indexes.sql");
        }
        populateOriginalRefs(db, g_refs);
        insertOriginalDispatches(db, g_dispatches, ChunkedInserts);
        if constexpr (SupplementalIndexes && DeferredIndexes) {
            executeSqlFile(db, "rocpd_indexes.sql");
            vacuumAfterDeferredIndexes(db);
        }
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(g_dispatches.size()));
    // Write benchmarks: do not persist DB files (only read benchmarks leave *.db for inspection).
    std::remove(kDb.c_str());
}

template <DispatchExtractOrder Order, bool ChunkedInserts, bool NoFKExclAccess,
          bool SupplementalIndexes, bool DeferredIndexes>
void BM_Write_Combined(benchmark::State& state) {
    ensureDataLoaded(Order);
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
            vacuumAfterDeferredIndexes(db);
        }
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() *
                            static_cast<int64_t>(g_dispatches.size()));
    // Write benchmarks: do not persist DB files (only read benchmarks leave *.db for inspection).
    std::remove(kDb.c_str());
}

template <DispatchExtractOrder Order, bool SupplementalIndexes, bool DeferredIndexes>
void BM_Read_Combined(benchmark::State& state) {
    ensureDataLoaded(Order);
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
        vacuumAfterDeferredIndexes(db_load);
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
    // kDb left on disk for EXPLAIN / tooling; write benchmarks delete their *.db.
}

template <DispatchExtractOrder Order, bool SupplementalIndexes, bool DeferredIndexes>
void BM_Read_Original(benchmark::State& state) {
    ensureDataLoaded(Order);
    const std::string kDb = std::string(state.name()) + ".db";
    sqlite3* db_load = createDatabase("rocpd_tables.sql", kDb, true, false);
    if constexpr (SupplementalIndexes && !DeferredIndexes) {
        executeSqlFile(db_load, "rocpd_indexes.sql");
    }
    populateOriginalRefs(db_load, g_refs);
    insertOriginalDispatches(db_load, g_dispatches, false);
    if constexpr (SupplementalIndexes && DeferredIndexes) {
        executeSqlFile(db_load, "rocpd_indexes.sql");
        vacuumAfterDeferredIndexes(db_load);
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
    // kDb left on disk for EXPLAIN / tooling; write benchmarks delete their *.db.
}

// Registration order: group by DispatchExtractOrder (ByStart → ById → Unordered).
// Within each group: original reads, combined reads, original writes, combined writes — so a
// full suite reloads the fixture at most three times instead of alternating order every case.

#define BM_READ_ORIGINAL_FOR_ORDER(OrderEnum, NameSuffix)                                     \
    BENCHMARK_TEMPLATE(BM_Read_Original, OrderEnum, false, false)                             \
        ->Name("BM_Read_Original_" NameSuffix)                                                \
        ->Unit(benchmark::kMillisecond);                                                      \
    BENCHMARK_TEMPLATE(BM_Read_Original, OrderEnum, true, false)                              \
        ->Name("BM_Read_Original_Indexed_Initial_" NameSuffix)                                \
        ->Unit(benchmark::kMillisecond);                                                      \
    BENCHMARK_TEMPLATE(BM_Read_Original, OrderEnum, true, true)                                 \
        ->Name("BM_Read_Original_Indexed_Deferred_" NameSuffix)                               \
        ->Unit(benchmark::kMillisecond);

#define BM_READ_COMBINED_FOR_ORDER(OrderEnum, NameSuffix)                                     \
    BENCHMARK_TEMPLATE(BM_Read_Combined, OrderEnum, false, false)                             \
        ->Name("BM_Read_Combined_" NameSuffix)                                                \
        ->Unit(benchmark::kMillisecond);                                                      \
    BENCHMARK_TEMPLATE(BM_Read_Combined, OrderEnum, true, false)                              \
        ->Name("BM_Read_Combined_Indexed_Initial_" NameSuffix)                                \
        ->Unit(benchmark::kMillisecond);                                                      \
    BENCHMARK_TEMPLATE(BM_Read_Combined, OrderEnum, true, true)                                 \
        ->Name("BM_Read_Combined_Indexed_Deferred_" NameSuffix)                               \
        ->Unit(benchmark::kMillisecond);

#define BM_WRITE_ORIGINAL_ALL(OrderEnum, NameSuffix)                                        \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, false, false, false, false)              \
        ->Name("BM_Write_Original_" NameSuffix)                                               \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, true, false, false, false)               \
        ->Name("BM_Write_Original_ChunkedInserts_" NameSuffix)                                \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, false, true, false, false)               \
        ->Name("BM_Write_Original_NoFKExclAccess_" NameSuffix)                                \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, true, true, false, false)                 \
        ->Name("BM_Write_Original_NoFKExclAccess_ChunkedInserts_" NameSuffix)                \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, false, false, true, false)               \
        ->Name("BM_Write_Original_Indexed_Initial_" NameSuffix)                               \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, true, false, true, false)                \
        ->Name("BM_Write_Original_Indexed_Initial_ChunkedInserts_" NameSuffix)                \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, false, true, true, false)               \
        ->Name("BM_Write_Original_Indexed_Initial_NoFKExclAccess_" NameSuffix)               \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, true, true, true, false)                \
        ->Name("BM_Write_Original_Indexed_Initial_NoFKExclAccess_ChunkedInserts_" NameSuffix) \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, false, false, true, true)                \
        ->Name("BM_Write_Original_Indexed_Deferred_" NameSuffix)                              \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, true, false, true, true)                 \
        ->Name("BM_Write_Original_Indexed_Deferred_ChunkedInserts_" NameSuffix)              \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, false, true, true, true)                 \
        ->Name("BM_Write_Original_Indexed_Deferred_NoFKExclAccess_" NameSuffix)              \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Original, OrderEnum, true, true, true, true)                  \
        ->Name("BM_Write_Original_Indexed_Deferred_NoFKExclAccess_ChunkedInserts_" NameSuffix) \
        ->Unit(benchmark::kMillisecond);

#define BM_WRITE_COMBINED_ALL(OrderEnum, NameSuffix)                                          \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, false, false, false, false)                \
        ->Name("BM_Write_Combined_" NameSuffix)                                               \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, true, false, false, false)               \
        ->Name("BM_Write_Combined_ChunkedInserts_" NameSuffix)                                \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, false, true, false, false)               \
        ->Name("BM_Write_Combined_NoFKExclAccess_" NameSuffix)                                \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, true, true, false, false)                 \
        ->Name("BM_Write_Combined_NoFKExclAccess_ChunkedInserts_" NameSuffix)                \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, false, false, true, false)               \
        ->Name("BM_Write_Combined_Indexed_Initial_" NameSuffix)                               \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, true, false, true, false)                \
        ->Name("BM_Write_Combined_Indexed_Initial_ChunkedInserts_" NameSuffix)                \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, false, true, true, false)               \
        ->Name("BM_Write_Combined_Indexed_Initial_NoFKExclAccess_" NameSuffix)               \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, true, true, true, false)                \
        ->Name("BM_Write_Combined_Indexed_Initial_NoFKExclAccess_ChunkedInserts_" NameSuffix) \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, false, false, true, true)                \
        ->Name("BM_Write_Combined_Indexed_Deferred_" NameSuffix)                              \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, true, false, true, true)                 \
        ->Name("BM_Write_Combined_Indexed_Deferred_ChunkedInserts_" NameSuffix)              \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, false, true, true, true)                 \
        ->Name("BM_Write_Combined_Indexed_Deferred_NoFKExclAccess_" NameSuffix)              \
        ->Unit(benchmark::kMillisecond);                                                     \
    BENCHMARK_TEMPLATE(BM_Write_Combined, OrderEnum, true, true, true, true)                  \
        ->Name("BM_Write_Combined_Indexed_Deferred_NoFKExclAccess_ChunkedInserts_" NameSuffix) \
        ->Unit(benchmark::kMillisecond);

BM_READ_ORIGINAL_FOR_ORDER(DispatchExtractOrder::ByStart, "OrderByStart")
BM_READ_COMBINED_FOR_ORDER(DispatchExtractOrder::ByStart, "OrderByStart")
BM_WRITE_ORIGINAL_ALL(DispatchExtractOrder::ByStart, "OrderByStart")
BM_WRITE_COMBINED_ALL(DispatchExtractOrder::ByStart, "OrderByStart")

BM_READ_ORIGINAL_FOR_ORDER(DispatchExtractOrder::ById, "OrderById")
BM_READ_COMBINED_FOR_ORDER(DispatchExtractOrder::ById, "OrderById")
BM_WRITE_ORIGINAL_ALL(DispatchExtractOrder::ById, "OrderById")
BM_WRITE_COMBINED_ALL(DispatchExtractOrder::ById, "OrderById")

BM_READ_ORIGINAL_FOR_ORDER(DispatchExtractOrder::Unordered, "OrderByUnspecified")
BM_READ_COMBINED_FOR_ORDER(DispatchExtractOrder::Unordered, "OrderByUnspecified")
BM_WRITE_ORIGINAL_ALL(DispatchExtractOrder::Unordered, "OrderByUnspecified")
BM_WRITE_COMBINED_ALL(DispatchExtractOrder::Unordered, "OrderByUnspecified")

#undef BM_READ_ORIGINAL_FOR_ORDER
#undef BM_READ_COMBINED_FOR_ORDER
#undef BM_WRITE_ORIGINAL_ALL
#undef BM_WRITE_COMBINED_ALL

}  // namespace
