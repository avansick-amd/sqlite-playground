# Code Refactoring Plan

## Current State
- Single monolithic file: `main.cpp` (847 lines)
- All functionality in one file

## Proposed Modular Structure

```
source/
├── types.h / types.cpp                    # Data structures (KernelDispatch, RefData)
├── data_loader.h / data_loader.cpp        # Extract data from ROCpd database
├── db_helpers.h / db_helpers.cpp          # Database creation and configuration
├── schema_original.h / schema_original.cpp # Original schema operations
├── schema_optimized.h / schema_optimized.cpp # Optimized schema operations
├── schema_combined.h / schema_combined.cpp # Combined schema operations
└── benchmarks.cpp                         # Benchmark definitions using Google Benchmark

main.cpp                                    # Entry point, minimal glue code
```

## Module Responsibilities

### types.h/cpp
- `GUID` constant
- `struct KernelDispatch` - kernel dispatch event data
- `struct RefData` - reference data from database

### data_loader.h/cpp
- `extractRealData()` - load data from ROCpd database file
- Populates KernelDispatch vector and RefData

### db_helpers.h/cpp
- `createDatabase()` - create database with schema and configure PRAGMAs

### schema_original.h/cpp
- `populateOriginalRefs()` - populate reference tables
- `insertOriginalDispatches()` - insert dispatch records
- `readOriginalDispatches()` - query and read dispatches

### schema_optimized.h/cpp
- Same functions as original but for optimized schema

### schema_combined.h/cpp
- Same functions but builds context table for deduplication

### benchmarks.cpp
- All `BM_*` benchmark functions
- Uses functions from schema modules

### main.cpp (new, minimal)
```cpp
#include <benchmark/benchmark.h>

// Entry point
BENCHMARK_MAIN();
```

## Benefits

1. **Maintainability**: Each schema in its own file
2. **Readability**: Clear module boundaries
3. **Testability**: Can test individual modules
4. **Reusability**: Schema code can be reused outside benchmarks
5. **Build Speed**: Parallel compilation of modules

## Implementation Steps

1. Create header files with function declarations ✓
2. Create implementation (.cpp) files
3. Update CMakeLists.txt to compile all sources
4. Test that everything builds and runs
5. Remove old monolithic main.cpp

## Files to Create (not yet done)

- [ ] source/data_loader.cpp
- [ ] source/db_helpers.cpp
- [ ] source/schema_original.cpp
- [ ] source/schema_optimized.cpp
- [ ] source/schema_combined.cpp
- [ ] source/benchmarks.cpp
- [ ] new main.cpp (minimal)
