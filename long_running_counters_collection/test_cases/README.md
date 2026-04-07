# test_cases

## `proposal*_indexes_for_counters_collection.sql`

These files list the **secondary indexes SQLite chose** when planning:

```sql
EXPLAIN QUERY PLAN SELECT * FROM counters_collection;
```

against:

- `../bench_copies/02_proposal2_event_pmc_dispatch.db`
- `../bench_copies/03_proposal3_recommended.db`

On both proposal databases, the join graph uses **rowid PRIMARY KEY lookups** on most dimension tables; only **`rocpd_kernel_dispatch_<GUID>`** and **`rocpd_pmc_event_<GUID>`** use named indexes in this plan.

**Not included:** `idx_cc_exp_kernel_dispatch_guid_event` exists on both DBs but is **not** used for this query plan (the planner scans `kernel_dispatch` via the other dispatch index).

GUID in the DDL matches this capture: `00002e0d_9533_7533_b0f4_7d200442d314`. Replace for other `.db` files.

## `extract_counters_collection_plan_indexes.py`

Regenerates the `proposal*.sql` files by re-running `EXPLAIN QUERY PLAN` and pulling `sql` from `sqlite_master`.
