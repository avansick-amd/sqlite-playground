# Benchmark analysis

Source: `results.json`

## Benchmark run summary

- **Repetitions per case:** 3 (from `--benchmark_repetitions=3`)
- **Executable:** `./benchmark_schema`
- **CPU:** 43 logical CPUs @ ~2396 MHz; CPU frequency scaling: **off**
- **Caches:** L1 Data 64 KiB; L1 Instruction 64 KiB; L2 Unified 512 KiB; L3 Unified 16384 KiB
- **Load average (1/5/15 min):** 0.59, 0.46, 0.40
- **JSON capture time:** 2026-03-26T15:18:30+00:00
- **Google Benchmark library build:** `debug` *(Debug affects timing; prefer Release for comparisons)*

## Write benchmarks

Baseline: `BM_Write_Original`. Δ and % use **wall (real) time** vs baseline wall time.

| Test name | Wall time (ms) | CPU time (ms) | Δ wall vs baseline | % vs baseline |
| --- | ---: | ---: | ---: | ---: |
| BM_Write_Combined_NoFKExclAccess_ChunkedInserts | 346.183 | 142.784 | -166.124 | -32.43% |
| BM_Write_Original_NoFKExclAccess | 370.794 | 155.912 | -141.513 | -27.62% |
| BM_Write_Combined_NoFKExclAccess | 378.008 | 185.305 | -134.299 | -26.21% |
| BM_Write_Combined_ChunkedInserts | 384.470 | 176.115 | -127.837 | -24.95% |
| BM_Write_Original_NoFKExclAccess_ChunkedInserts | 403.740 | 140.428 | -108.567 | -21.19% |
| BM_Write_Combined | 413.055 | 214.439 | -99.252 | -19.37% |
| BM_Write_Original_ChunkedInserts | 446.696 | 180.044 | -65.611 | -12.81% |
| BM_Write_Combined_Indexed_Deferred_NoFKExclAccess_ChunkedInserts | 461.827 | 186.717 | -50.480 | -9.85% |
| BM_Write_Combined_Indexed_Deferred_NoFKExclAccess | 473.372 | 227.836 | -38.935 | -7.60% |
| **BM_Write_Original** | **512.307** | **229.060** | **+0.000** | **+0.00%** |
| BM_Write_Combined_Indexed_Deferred | 525.759 | 258.769 | +13.452 | +2.63% |
| BM_Write_Combined_Indexed_Deferred_ChunkedInserts | 551.310 | 226.987 | +39.003 | +7.61% |
| BM_Write_Original_Indexed_Deferred_NoFKExclAccess_ChunkedInserts | 577.130 | 220.384 | +64.823 | +12.65% |
| BM_Write_Combined_Indexed_Initial_NoFKExclAccess_ChunkedInserts | 601.550 | 248.941 | +89.243 | +17.42% |
| BM_Write_Combined_Indexed_Initial_ChunkedInserts | 618.397 | 264.968 | +106.090 | +20.71% |
| BM_Write_Original_Indexed_Deferred_NoFKExclAccess | 625.557 | 268.157 | +113.250 | +22.11% |
| BM_Write_Combined_Indexed_Initial_NoFKExclAccess | 651.919 | 295.787 | +139.612 | +27.25% |
| BM_Write_Original_Indexed_Deferred_ChunkedInserts | 670.144 | 278.617 | +157.837 | +30.81% |
| BM_Write_Original_Indexed_Deferred | 720.359 | 309.482 | +208.052 | +40.61% |
| BM_Write_Combined_Indexed_Initial | 766.136 | 335.871 | +253.829 | +49.55% |
| BM_Write_Original_Indexed_Initial_NoFKExclAccess_ChunkedInserts | 930.253 | 336.249 | +417.946 | +81.58% |
| BM_Write_Original_Indexed_Initial_ChunkedInserts | 987.254 | 391.026 | +474.947 | +92.71% |
| BM_Write_Original_Indexed_Initial_NoFKExclAccess | 1005.684 | 411.274 | +493.377 | +96.30% |
| BM_Write_Original_Indexed_Initial | 1823.736 | 570.977 | +1311.429 | +255.98% |

## Read benchmarks

Baseline: `BM_Read_Original`. Δ and % use **wall (real) time** vs baseline wall time.

| Test name | Wall time (ms) | CPU time (ms) | Δ wall vs baseline | % vs baseline |
| --- | ---: | ---: | ---: | ---: |
| BM_Read_Combined_Indexed_Initial | 94.301 | 33.269 | -4.218 | -4.28% |
| **BM_Read_Original** | **98.518** | **30.824** | **+0.000** | **+0.00%** |
| BM_Read_Original_Indexed_Initial | 132.593 | 41.045 | +34.074 | +34.59% |
| BM_Read_Combined | 138.002 | 55.059 | +39.483 | +40.08% |
| BM_Read_Original_Indexed_Deferred | 290.503 | 60.784 | +191.984 | +194.87% |
| BM_Read_Combined_Indexed_Deferred | 291.168 | 65.250 | +192.649 | +195.55% |

## Benchmark name reference

Names follow `BM_<Operation>_<Schema>[_<Variant>…]`. Baselines for this report are `BM_Write_Original` (writes) and `BM_Read_Original` (reads).

| Part | Meaning |
| --- | --- |
| `Write` / `Read` | Timed operation: bulk insert + indexes vs query workload. |
| `Original` | `rocpd_tables.sql` schema (original layout). |
| `Combined` | `rocpd_tables_combined.sql` (deduplicated dispatch context). |
| `ChunkedInserts` | Dispatches inserted with multi-row `INSERT` batches (vs one row per statement). |
| `NoFKExclAccess` | **Write benchmarks only:** `PRAGMA foreign_keys = OFF` and `PRAGMA locking_mode = EXCLUSIVE` during `createDatabase` / load. Read benchmarks never use this variant; load and timed read both use FK on + NORMAL locking. |
| `Indexed_Initial` | Supplemental index SQL applied **before** dispatch inserts. |
| `Indexed_Deferred` | Supplemental index SQL applied **after** dispatch inserts. |
