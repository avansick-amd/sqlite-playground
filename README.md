# SQLite Schema Benchmark for ROCpd

Performance benchmarking of SQLite database schemas for ROCm GPU profiling data (ROCpd).

## Overview

This project compares two schema designs for storing GPU kernel dispatch profiling data:
- **Original Schema**: Denormalized with redundant columns (7 foreign keys)
- **Combined Schema**: Context-deduplicated design (2 foreign keys)

Real-world test data: 62,287 kernel dispatch events from a 1.2 GB ROCpd database.

## Key Results

🏆 **Write Performance**: Combined schema is **7% faster** (688k items/s vs 638k items/s)
🏆 **Read Performance**: Original schema is **14% faster** (6.09M items/s vs 5.37M items/s)
⚠️ **Index Impact**: Extra indexes harm writes significantly (-92% for Original, -43% for Combined)

See [BENCHMARK_RESULTS.md](BENCHMARK_RESULTS.md) for detailed analysis.

---

## Prerequisites

### System Requirements
- Linux or macOS
- CMake 3.15 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 2017+)
- SQLite3 development libraries
- Git (for downloading Google Benchmark)

### Installing Dependencies

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libsqlite3-dev git
```

#### Fedora/RHEL
```bash
sudo dnf install -y gcc-c++ cmake sqlite-devel git
```

#### macOS (Homebrew)
```bash
brew install cmake sqlite3
```

---

## Project Structure

```
sqlite-benchmark/
├── benchmark_data/
│   └── rocpd-7389.db                        # Real profiling data (1.2 GB)
├── schemas/
│   ├── rocpd_tables.sql                     # Original denormalized schema
│   ├── rocpd_tables_combined.sql            # Context-deduplicated schema
│   ├── rocpd_tables_indexed.sql             # Original + performance indexes
│   └── rocpd_tables_combined_indexed.sql    # Combined + performance indexes
├── source/                                  # Modular source code
│   ├── types.h/cpp                          # Data structures (KernelDispatch, RefData)
│   ├── data_loader.h/cpp                    # Extract data from ROCpd database
│   ├── db_helpers.h/cpp                     # Database creation utilities
│   ├── schema_original.h/cpp                # Original schema operations
│   ├── schema_combined.h/cpp                # Combined schema operations
│   └── benchmarks.h/cpp                     # Benchmark definitions
├── build/                                   # Build directory (auto-generated)
├── main.cpp                                 # Main entry point
├── BENCHMARK_RESULTS.md                     # Detailed performance analysis
├── CMakeLists.txt                           # Build configuration
├── README.md                                # This file
└── .gitignore                               # Git exclusions
```

---

## Building the Project

### 1. Clone or Navigate to Project Directory

```bash
cd /path/to/sqlite-benchmark
```

### 2. Configure with CMake

```bash
cmake -S . -B build
```

### 3. Compile

```bash
cmake --build build -j$(nproc)
```

---

## Running Benchmarks

### Run All Benchmarks

```bash
cd build
./benchmark_schema
```

**Output example:**
```
Extracted: 62287 dispatches, 10 threads, 4 agents, 5 queues, 1 streams, 13131 kernels
------------------------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations UserCounters...
------------------------------------------------------------------------------------
BM_Write_Original                206 ms         97.6 ms            6 items_per_second=638k/s
BM_Write_Combined                192 ms         90.5 ms            8 items_per_second=688k/s
BM_Read_Original                10.2 ms         10.2 ms           67 items_per_second=6.09M/s
BM_Read_Combined                11.6 ms         11.6 ms           59 items_per_second=5.37M/s
...
```

### Run Specific Benchmarks

Filter by name pattern:
```bash
# Only write benchmarks
./benchmark_schema --benchmark_filter=BM_Write

# Only read benchmarks
./benchmark_schema --benchmark_filter=BM_Read

# Only Original schema
./benchmark_schema --benchmark_filter=.*Original

# Only Combined schema
./benchmark_schema --benchmark_filter=.*Combined

# Only indexed variants
./benchmark_schema --benchmark_filter=.*Indexed
```

### Benchmark Output Options

```bash
# Save results to JSON
./benchmark_schema --benchmark_out=results.json --benchmark_out_format=json

# Save results to CSV
./benchmark_schema --benchmark_out=results.csv --benchmark_out_format=csv

# Display more detailed timing stats
./benchmark_schema --benchmark_repetitions=10

# Run only fast benchmarks (fewer iterations)
./benchmark_schema --benchmark_min_time=0.1
```

---

## Understanding the Benchmarks

### Benchmark Naming Convention

Format: `BM_<Operation>_<Schema>_<Variant>`

- **Operation**: `Write` or `Read`
- **Schema**: `Original` or `Combined`
- **Variant** (optional): `NoFK` (foreign keys disabled), `Indexed` (additional indexes)

### What Each Benchmark Tests

| Benchmark | Schema File | Test Description |
|-----------|-------------|------------------|
| `BM_Write_Original` | rocpd_tables.sql | Insert 62,287 dispatches (7 FKs) |
| `BM_Write_Combined` | rocpd_tables_combined.sql | Insert 62,287 dispatches (2 FKs) |
| `BM_Write_Original_NoFK` | rocpd_tables.sql | Insert without FK validation |
| `BM_Write_Combined_NoFK` | rocpd_tables_combined.sql | Insert without FK validation |
| `BM_Write_Original_Indexed` | rocpd_tables_indexed.sql | Insert with extra indexes |
| `BM_Write_Combined_Indexed` | rocpd_tables_combined_indexed.sql | Insert with extra indexes |
| `BM_Read_Original` | rocpd_tables.sql | Full table scan with JOIN |
| `BM_Read_Combined` | rocpd_tables_combined.sql | Full table scan with 2 JOINs |
| `BM_Read_Original_Indexed` | rocpd_tables_indexed.sql | Full table scan (indexed) |
| `BM_Read_Combined_Indexed` | rocpd_tables_combined_indexed.sql | Full table scan (indexed) |

### Metrics Explained

- **Time**: Wall-clock time (real elapsed time)
- **CPU**: Actual CPU processing time
- **Iterations**: Number of times the benchmark ran (auto-determined for statistical confidence)
- **items_per_second**: Throughput (62,287 dispatches / time)

**Example:**
```
BM_Write_Combined    192 ms    90.5 ms    8    items_per_second=688k/s
```
- Took 192ms real time, 90.5ms CPU time
- Ran 8 times for accuracy
- Processed 688,000 items per second

---

## Schema Details

### Original Schema (rocpd_tables.sql)

**Design**: Denormalized with redundant `nid`, `pid` columns

```sql
CREATE TABLE rocpd_kernel_dispatch (
    id INTEGER PRIMARY KEY,
    nid INTEGER,              -- Redundant (derivable from tid)
    pid INTEGER,              -- Redundant (derivable from tid)
    tid INTEGER,
    agent_id INTEGER,
    kernel_id INTEGER,
    dispatch_id INTEGER,
    queue_id INTEGER,
    stream_id INTEGER,
    start BIGINT,
    "end" BIGINT,
    -- ... dimension fields ...
    FOREIGN KEY (nid) REFERENCES rocpd_info_node (id),
    FOREIGN KEY (pid) REFERENCES rocpd_info_process (id),
    FOREIGN KEY (tid) REFERENCES rocpd_info_thread (id),
    FOREIGN KEY (agent_id) REFERENCES rocpd_info_agent (id),
    FOREIGN KEY (kernel_id) REFERENCES rocpd_info_kernel_symbol (id),
    FOREIGN KEY (queue_id) REFERENCES rocpd_info_queue (id),
    FOREIGN KEY (stream_id) REFERENCES rocpd_info_stream (id)
);
```

**Pros**: Faster reads (no JOINs needed for context)
**Cons**: Slower writes (7 FK validations), larger storage

### Combined Schema (rocpd_tables_combined.sql)

**Design**: Context deduplication via separate context table

```sql
CREATE TABLE rocpd_dispatch_context (
    id INTEGER PRIMARY KEY,
    nid INTEGER,
    pid INTEGER,
    tid INTEGER,
    agent_id INTEGER,
    agent_type TEXT,
    agent_name TEXT,
    queue_id INTEGER,
    queue_name TEXT,
    stream_id INTEGER,
    stream_name TEXT,
    UNIQUE(nid, pid, tid, agent_id, queue_id, stream_id)
);

CREATE TABLE rocpd_kernel_dispatch (
    id INTEGER PRIMARY KEY,
    context_id INTEGER,       -- Single FK replaces 6 columns
    kernel_id INTEGER,
    dispatch_id INTEGER,
    start BIGINT,
    "end" BIGINT,
    -- ... dimension fields ...
    FOREIGN KEY (context_id) REFERENCES rocpd_dispatch_context (id),
    FOREIGN KEY (kernel_id) REFERENCES rocpd_info_kernel_symbol (id)
);
```

**Pros**: Faster writes (2 FK validations), smaller storage
**Cons**: Slightly slower reads (needs JOIN to context table)

---

## Customization

### Using Your Own Database

Replace `benchmark_data/rocpd-7389.db` with your own ROCpd database:

```bash
cp /path/to/your/rocpd.db benchmark_data/rocpd-7389.db
```

**Note**: Database must have the ROCpd schema with GUID suffix `eb95343b77a98cb86f9e54335edbb3f4`.

### Modifying Benchmark Parameters

Edit `source/db_helpers.cpp`:

```cpp
// Modify PRAGMA settings
sqlite3_exec(db, "PRAGMA synchronous = OFF;", ...);    // Faster writes
sqlite3_exec(db, "PRAGMA journal_mode = MEMORY;", ...); // In-memory journal
```

Edit `source/data_loader.cpp`:

```cpp
// Change which database to extract from
extractRealData("benchmark_data/rocpd-7389.db", dispatches, refs);
```

### Creating New Schemas

1. Create new schema file in `schemas/`: `schemas/rocpd_tables_custom.sql`
2. Add to `CMakeLists.txt`:
   ```cmake
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/schemas/rocpd_tables_custom.sql
                  ${CMAKE_CURRENT_BINARY_DIR}/rocpd_tables_custom.sql
                  COPYONLY)
   ```
3. Create `source/schema_custom.h/cpp` with populate/insert/read functions
4. Add benchmark functions to `source/benchmarks.cpp`
5. Register benchmark: `BENCHMARK(BM_Write_Custom)->Unit(benchmark::kMillisecond);`

---

## Troubleshooting

### Build Errors

**Error**: `Could NOT find SQLite3`
```bash
# Ubuntu/Debian
sudo apt-get install libsqlite3-dev

### Runtime Errors

**Error**: `Failed to open benchmark_data/rocpd-7389.db`
```bash
# Check file exists
ls -lh benchmark_data/rocpd-7389.db

# Check you're in build directory
pwd  # Should be: /path/to/sqlite-benchmark/build
```

**Error**: `no such table: rocpd_kernel_dispatch_<GUID>`
- Database schema mismatch
- Check GUID in `source/types.cpp` matches your database

### Performance Issues

**CPU Scaling Warning**: `CPU scaling is enabled, benchmark measurements may be noisy`
```bash
# Disable CPU frequency scaling (Linux)
sudo cpupower frequency-set --governor performance

# Re-enable after benchmarking
sudo cpupower frequency-set --governor powersave
```

**Noisy Results**: Run with more iterations
```bash
./benchmark_schema --benchmark_repetitions=20
```

---

## License

This project is for benchmarking and research purposes.

---

## References

- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [SQLite Performance Tips](https://www.sqlite.org/performance.html)
- [ROCm Profiler Documentation](https://rocmdocs.amd.com/)
- Full analysis: [BENCHMARK_RESULTS.md](BENCHMARK_RESULTS.md)

---

## Quick Start Summary

```bash
# 1. Install dependencies
sudo apt-get install build-essential cmake libsqlite3-dev git

# 2. Build
cmake -S . -B build
cmake --build build -j$(nproc)

# 3. Run
./benchmark_schema

# 4. View results
cat ../BENCHMARK_RESULTS.md
```

**Questions?** Check [BENCHMARK_RESULTS.md](BENCHMARK_RESULTS.md) for detailed analysis.
