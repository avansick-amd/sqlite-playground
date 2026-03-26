# Benchmark analysis

Source: `results.json`

## Benchmark run summary

- **Repetitions per case:** 10 (from `--benchmark_repetitions=10`)
- **Executable:** `./benchmark_schema`
- **CPU:** 43 logical CPUs @ ~2396 MHz; CPU frequency scaling: **off**
- **Caches:** L1 Data 64 KiB; L1 Instruction 64 KiB; L2 Unified 512 KiB; L3 Unified 16384 KiB
- **Load average (1/5/15 min):** 0.22, 0.28, 0.44
- **JSON capture time:** 2026-03-26T16:23:27+00:00
- **Google Benchmark library build:** `debug` *(Debug affects timing; prefer Release for comparisons)*

## Write benchmarks

Baseline: `BM_Write_Original`. Δ and % use **wall (real) time** vs baseline wall time.

| Test name | Wall time (ms) | CPU time (ms) | Δ wall vs baseline | % vs baseline |
| --- | ---: | ---: | ---: | ---: |
| BM_Write_Combined_NoFKExclAccess_ChunkedInserts | 331.775 | 139.936 | -149.481 | -31.06% |
| BM_Write_Original_NoFKExclAccess_ChunkedInserts | 366.134 | 137.370 | -115.122 | -23.92% |
| BM_Write_Combined_NoFKExclAccess | 383.023 | 185.550 | -98.233 | -20.41% |
| BM_Write_Combined_ChunkedInserts | 391.553 | 178.986 | -89.702 | -18.64% |
| BM_Write_Original_NoFKExclAccess | 407.099 | 181.771 | -74.157 | -15.41% |
| BM_Write_Combined | 431.838 | 219.345 | -49.418 | -10.27% |
| BM_Write_Original_ChunkedInserts | 438.743 | 190.264 | -42.512 | -8.83% |
| BM_Write_Combined_Indexed_Deferred_NoFKExclAccess_ChunkedInserts | 440.105 | 184.090 | -41.151 | -8.55% |
| BM_Write_Combined_Indexed_Deferred_NoFKExclAccess | 476.943 | 222.981 | -4.313 | -0.90% |
| **BM_Write_Original** | **481.255** | **234.301** | **+0.000** | **+0.00%** |
| BM_Write_Combined_Indexed_Deferred_ChunkedInserts | 502.148 | 215.560 | +20.892 | +4.34% |
| BM_Write_Combined_Indexed_Deferred | 538.353 | 256.600 | +57.098 | +11.86% |
| BM_Write_Combined_Indexed_Initial_NoFKExclAccess_ChunkedInserts | 578.261 | 234.321 | +97.006 | +20.16% |
| BM_Write_Combined_Indexed_Initial_NoFKExclAccess | 624.307 | 278.218 | +143.052 | +29.72% |
| BM_Write_Original_Indexed_Deferred_NoFKExclAccess_ChunkedInserts | 630.053 | 224.939 | +148.797 | +30.92% |
| BM_Write_Combined_Indexed_Initial_ChunkedInserts | 634.130 | 269.216 | +152.874 | +31.77% |
| BM_Write_Original_Indexed_Deferred_NoFKExclAccess | 646.708 | 266.212 | +165.453 | +34.38% |
| BM_Write_Combined_Indexed_Initial | 685.750 | 316.304 | +204.495 | +42.49% |
| BM_Write_Original_Indexed_Deferred_ChunkedInserts | 693.503 | 286.021 | +212.248 | +44.10% |
| BM_Write_Original_Indexed_Deferred | 737.621 | 326.243 | +256.366 | +53.27% |
| BM_Write_Original_Indexed_Initial_NoFKExclAccess_ChunkedInserts | 828.078 | 327.854 | +346.822 | +72.07% |
| BM_Write_Original_Indexed_Initial_NoFKExclAccess | 904.449 | 391.901 | +423.194 | +87.94% |
| BM_Write_Original_Indexed_Initial_ChunkedInserts | 952.391 | 377.348 | +471.136 | +97.90% |
| BM_Write_Original_Indexed_Initial | 1024.236 | 459.526 | +542.980 | +112.83% |

## Read benchmarks

Baseline: `BM_Read_Original`. Δ and % use **wall (real) time** vs baseline wall time.

| Test name | Wall time (ms) | CPU time (ms) | Δ wall vs baseline | % vs baseline |
| --- | ---: | ---: | ---: | ---: |
| BM_Read_Combined | 84.383 | 30.903 | -26.871 | -24.15% |
| BM_Read_Combined_Indexed_Initial | 86.043 | 30.612 | -25.211 | -22.66% |
| **BM_Read_Original** | **111.254** | **47.848** | **+0.000** | **+0.00%** |
| BM_Read_Original_Indexed_Initial | 134.581 | 49.867 | +23.327 | +20.97% |
| BM_Read_Combined_Indexed_Deferred | 301.225 | 79.373 | +189.971 | +170.75% |
| BM_Read_Original_Indexed_Deferred | 328.818 | 82.633 | +217.564 | +195.56% |

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
