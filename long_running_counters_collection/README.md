# long_running_counters_collection

Notes for future runs (and future agents): this folder benchmarks reading the ROCpd view **`counters_collection`** on a large capture (`SQC_DCACHE_INFLIGHT_LEVEL_20548.db` and copies under `bench_copies/`).

**Assistants:** read **`AGENTS.md`** first for path mapping, file roles, and what was never committed vs recovered from the DBs.

## What is here

| Path | Role |
| --- | --- |
| `SQC_DCACHE_INFLIGHT_LEVEL_20548.db` | Primary capture (multi‑GB). A `*.db-journal` beside it means a transaction was not fully committed—treat as suspicious until `PRAGMA integrity_check` / clean close. |
| `bench_copies/` | Full-file copies with extra indexes: `01_control.db`, `02_proposal2_event_pmc_dispatch.db`, `03_proposal3_recommended.db`. Timings in `timings.csv` refer to these phases. |
| `time_counters_collection_full_scan.py` | Times `SELECT * FROM counters_collection` with `fetchmany` (constant Python memory). |
| `test_cases/` | SQL artifacts: indexes that **actually participate** in the planner’s execution of `SELECT * FROM counters_collection` on each proposal DB (see `test_cases/README.md`). |
| `../rocpd-302983-schema/queries/index_proposals_recovered_from_bench_copies.sql` | All supplemental `CREATE INDEX` statements recovered from `sqlite_master` (including indexes **not** chosen for the view query). |

## Methodology we care about

- **Comparing index shapes** on the **same schema** (full DB copy + `CREATE INDEX`) is not the same as building a tiny materialized subset and indexing that: cardinalities and plans change. Prefer full copies like `bench_copies` when the question is “how does this behave on the real file?”
- To see which indexes matter for the view, use **`EXPLAIN QUERY PLAN SELECT * FROM counters_collection`** on the **attached DB**. Regenerate `test_cases/*.sql` if the schema or SQLite version changes.

## Regenerating `test_cases` SQL (optional)

```bash
python3 test_cases/extract_counters_collection_plan_indexes.py
```

(If that script is missing, recreate it from the comments in `test_cases/README.md`.)
