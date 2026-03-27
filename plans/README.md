# Query plans (`EXPLAIN QUERY PLAN`)

After running the read benchmarks with databases left on disk, generate Markdown with the planner output for `readOriginalDispatches`.

The three benchmark runs (original schema) produce these files next to the working directory where you run `benchmark_schema`:

| File | Meaning |
| --- | --- |
| `BM_Read_Original.db` | No supplemental indexes |
| `BM_Read_Original_Indexed_Initial.db` | `rocpd_indexes.sql` applied **before** inserts |
| `BM_Read_Original_Indexed_Deferred.db` | `rocpd_indexes.sql` applied **after** inserts |

**Default:** run with no DB arguments to explain all three (from the current directory):

```bash
chmod +x scripts/explain_read_plans.sh
./scripts/explain_read_plans.sh -a
```

If the `.db` files live elsewhere (e.g. build dir):

```bash
./scripts/explain_read_plans.sh -a -d ../build-s93-vm7
```

Or pass paths explicitly:

```bash
./scripts/explain_read_plans.sh -a ./BM_Read_Original.db ./BM_Read_Original_Indexed_Initial.db ./BM_Read_Original_Indexed_Deferred.db
```

Output defaults to **`./plans/read_original_explain.md`** in whatever directory you run the script from (the shell’s current directory at execution time).

- **`-a`** — run `ANALYZE` on each DB before `EXPLAIN QUERY PLAN` (fairer planner stats).
- **`-A`** — append a section that runs the **same `SELECT`** with **`.timer on`** and **`.scanstats on`**, then **keeps only the trailing summary** (the `QUERY PLAN (cycles=…)` / scan tree and **`Run Time:`** line). Result rows are omitted so the Markdown stays small.
- **`-o path`** — write to another file.

The SQL in `scripts/explain_read_plans.sh` must stay aligned with **`source/schema_original.cpp`** (`readOriginalDispatches`).
