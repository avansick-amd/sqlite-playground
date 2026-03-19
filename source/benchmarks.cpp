#include "benchmarks.h"
#include "data_loader.h"
#include "db_helpers.h"
#include "schema_original.h"
#include "schema_combined.h"
#include <benchmark/benchmark.h>
#include <cstdio>

// Global data storage
std::vector<KernelDispatch> g_dispatches;
RefData g_refs;
bool g_data_loaded = false;

// Helper to ensure data is loaded once
static void ensureDataLoaded() {
    if (!g_data_loaded) {
        extractRealData("benchmark_data/rocpd-7389.db", g_dispatches, g_refs);
        g_data_loaded = true;
    }
}

// ============ WRITE BENCHMARKS ============

static void BM_Write_Original(benchmark::State& state) {
    ensureDataLoaded();
    for (auto _ : state) {
        sqlite3* db = createDatabase("rocpd_tables.sql", "bench_orig.db");
        populateOriginalRefs(db, g_refs);
        insertOriginalDispatches(db, g_dispatches);
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() * g_dispatches.size());
    std::remove("bench_orig.db");
}

static void BM_Write_Combined(benchmark::State& state) {
    ensureDataLoaded();
    for (auto _ : state) {
        sqlite3* db = createDatabase("rocpd_tables_combined.sql", "bench_comb.db");
        populateCombinedRefs(db, g_refs, g_dispatches);
        insertCombinedDispatches(db, g_dispatches);
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() * g_dispatches.size());
    std::remove("bench_comb.db");
}

// ============ READ BENCHMARKS ============

static void BM_Read_Original(benchmark::State& state) {
    ensureDataLoaded();
    sqlite3* db = createDatabase("rocpd_tables.sql", "bench_read_orig.db");
    populateOriginalRefs(db, g_refs);
    insertOriginalDispatches(db, g_dispatches);

    for (auto _ : state) {
        benchmark::DoNotOptimize(readOriginalDispatches(db));
    }
    state.SetItemsProcessed(state.iterations() * g_dispatches.size());
    sqlite3_close(db);
    std::remove("bench_read_orig.db");
}

static void BM_Read_Combined(benchmark::State& state) {
    ensureDataLoaded();
    sqlite3* db = createDatabase("rocpd_tables_combined.sql", "bench_read_comb.db");
    populateCombinedRefs(db, g_refs, g_dispatches);
    insertCombinedDispatches(db, g_dispatches);

    for (auto _ : state) {
        benchmark::DoNotOptimize(readCombinedDispatches(db));
    }
    state.SetItemsProcessed(state.iterations() * g_dispatches.size());
    sqlite3_close(db);
    std::remove("bench_read_comb.db");
}

// ============ NO FK BENCHMARKS ============

static void BM_Write_Original_NoFK(benchmark::State& state) {
    ensureDataLoaded();
    for (auto _ : state) {
        sqlite3* db = createDatabase("rocpd_tables.sql", "bench_orig_nofk.db", false);
        populateOriginalRefs(db, g_refs);
        insertOriginalDispatches(db, g_dispatches);
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() * g_dispatches.size());
    std::remove("bench_orig_nofk.db");
}

static void BM_Write_Combined_NoFK(benchmark::State& state) {
    ensureDataLoaded();
    for (auto _ : state) {
        sqlite3* db = createDatabase("rocpd_tables_combined.sql", "bench_comb_nofk.db", false);
        populateCombinedRefs(db, g_refs, g_dispatches);
        insertCombinedDispatches(db, g_dispatches);
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() * g_dispatches.size());
    std::remove("bench_comb_nofk.db");
}

// ============ INDEXED SCHEMA BENCHMARKS ============

static void BM_Write_Original_Indexed(benchmark::State& state) {
    ensureDataLoaded();
    for (auto _ : state) {
        sqlite3* db = createDatabase("rocpd_tables_indexed.sql", "bench_orig_idx.db");
        populateOriginalRefs(db, g_refs);
        insertOriginalDispatches(db, g_dispatches);
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() * g_dispatches.size());
    std::remove("bench_orig_idx.db");
}

static void BM_Write_Combined_Indexed(benchmark::State& state) {
    ensureDataLoaded();
    for (auto _ : state) {
        sqlite3* db = createDatabase("rocpd_tables_combined_indexed.sql", "bench_comb_idx.db");
        populateCombinedRefs(db, g_refs, g_dispatches);
        insertCombinedDispatches(db, g_dispatches);
        sqlite3_close(db);
    }
    state.SetItemsProcessed(state.iterations() * g_dispatches.size());
    std::remove("bench_comb_idx.db");
}

static void BM_Read_Original_Indexed(benchmark::State& state) {
    ensureDataLoaded();
    sqlite3* db = createDatabase("rocpd_tables_indexed.sql", "bench_read_orig_idx.db");
    populateOriginalRefs(db, g_refs);
    insertOriginalDispatches(db, g_dispatches);

    for (auto _ : state) {
        benchmark::DoNotOptimize(readOriginalDispatches(db));
    }
    state.SetItemsProcessed(state.iterations() * g_dispatches.size());
    sqlite3_close(db);
    std::remove("bench_read_orig_idx.db");
}

static void BM_Read_Combined_Indexed(benchmark::State& state) {
    ensureDataLoaded();
    sqlite3* db = createDatabase("rocpd_tables_combined_indexed.sql", "bench_read_comb_idx.db");
    populateCombinedRefs(db, g_refs, g_dispatches);
    insertCombinedDispatches(db, g_dispatches);

    for (auto _ : state) {
        benchmark::DoNotOptimize(readCombinedDispatches(db));
    }
    state.SetItemsProcessed(state.iterations() * g_dispatches.size());
    sqlite3_close(db);
    std::remove("bench_read_comb_idx.db");
}

// ============ BENCHMARK REGISTRATION ============

BENCHMARK(BM_Write_Original)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Write_Original_NoFK)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Write_Combined)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Write_Combined_NoFK)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Write_Original_Indexed)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Write_Combined_Indexed)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Read_Original)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Read_Combined)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Read_Original_Indexed)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Read_Combined_Indexed)->Unit(benchmark::kMillisecond);
