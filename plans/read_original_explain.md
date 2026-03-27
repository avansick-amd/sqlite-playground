# EXPLAIN QUERY PLAN — `readOriginalDispatches`

Generated: 2026-03-26T18:12:17Z (UTC)

Query (must match `schema_original.cpp`):

```sql
SELECT d.id, d.nid, d.pid, d.tid, d.agent_id, d.kernel_id,
       d.dispatch_id, d.queue_id, d.stream_id, d.start, d."end",
       k.kernel_name
FROM rocpd_kernel_dispatch d
JOIN rocpd_info_kernel_symbol k ON d.kernel_id = k.id
ORDER BY d.start;
```

## BM_Read_Original.db

No supplemental indexes (only schema / implicit indexes).

Path: `/development/github_pulls/sqlite-playground/build-s93-vm7/BM_Read_Original.db`

Running `ANALYZE` …

### EXPLAIN QUERY PLAN

```
QUERY PLAN
|--SCAN d USING INDEX idx_dispatch_start
`--SEARCH k USING INTEGER PRIMARY KEY (rowid=?)
```

## BM_Read_Original_Indexed_Initial.db

Supplemental indexes from `rocpd_indexes.sql` applied **before** dispatch inserts.

Path: `/development/github_pulls/sqlite-playground/build-s93-vm7/BM_Read_Original_Indexed_Initial.db`

Running `ANALYZE` …

### EXPLAIN QUERY PLAN

```
QUERY PLAN
|--SCAN d USING INDEX idx_dispatch_start_kernel
`--SEARCH k USING INTEGER PRIMARY KEY (rowid=?)
```

## BM_Read_Original_Indexed_Deferred.db

Supplemental indexes from `rocpd_indexes.sql` applied **after** dispatch inserts.

Path: `/development/github_pulls/sqlite-playground/build-s93-vm7/BM_Read_Original_Indexed_Deferred.db`

Running `ANALYZE` …

### EXPLAIN QUERY PLAN

```
QUERY PLAN
|--SCAN d USING INDEX idx_dispatch_start_kernel
`--SEARCH k USING INTEGER PRIMARY KEY (rowid=?)
```

