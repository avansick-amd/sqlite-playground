# Handoff for assistants (read this when the user opens this folder)

## What this folder is

ROCpd **SQLite** work: benchmark **`SELECT * FROM counters_collection`** on a large capture, comparing **index shapes** on full DB copies (not a toy materialized subset).

## Paths

- **Host:** `/scratch/users/avansick/long_running_counters_collection`
- **Typical container mount:** `/development/long_running_counters_collection` (same files if `HOST_DEV_PATH` is the user’s scratch tree)

## Key files (do not “clean up” as unused)

| Item | Notes |
| --- | --- |
| `SQC_DCACHE_INFLIGHT_LEVEL_20548.db` | Main multi‑GB DB. Co-located `*.db-journal` ⇒ possible incomplete txn; suggest `integrity_check` if debugging corruption. |
| `bench_copies/01_control.db`, `02_*`, `03_*` | Full copies + extra indexes; `timings.csv` rows match these names (`control`, `proposal2_event_pmc_dispatch`, `proposal3_recommended`). |
| `time_counters_collection_full_scan.py` | Only tool here that times the view read (`fetchmany`). |
| `test_cases/proposal{2,3}_indexes_for_counters_collection.sql` | **`EXPLAIN QUERY PLAN SELECT * FROM counters_collection`** → which **named** indexes the planner uses on each proposal DB. |
| `test_cases/extract_counters_collection_plan_indexes.py` | Regenerates those SQL files from `bench_copies`. |

## Related paths outside this folder

- `../rocpd-302983-schema/rocpd-302983.schema.sql` — schema incl. `counters_collection` view.
- `../rocpd-302983-schema/queries/index_proposals_recovered_from_bench_copies.sql` — **all** supplemental `CREATE INDEX` from `sqlite_master` (includes indexes **not** used by the view plan, e.g. `idx_cc_exp_*`).
- `../github_pulls/sqlite-playground/` — **different** read benchmarks (`readRocpdSourceDispatchJoin`, etc.); not the counters view.

## Conventions already agreed with the user

- Prefer **full DB copy + indexes** for apples-to-apples vs production-like reads; subset-then-index changes planner behavior.
- “Participating indexes” for the view = from **`EXPLAIN QUERY PLAN`** on the **specific** `.db`, not every index in the file.

## If something is “missing”

Earlier chat-only ideas (approximate view SQL, `LIMIT` from one join leg, standalone proposal SQL) were **never saved**; index definitions were **recovered** from `sqlite_master` inside `bench_copies` and documented in `test_cases/` and `rocpd-302983-schema/queries/`.

See **`README.md`** for the same story in user-facing form.
