# Read query performance: Initial vs Deferred indexes

Context: **`BM_Read_Original_Indexed_Initial`** vs **`BM_Read_Original_Indexed_Deferred`** — `EXPLAIN QUERY PLAN` may match; runtime can still differ. Work through this list in order.

**SQLite note:** Structural comparison uses **`EXPLAIN QUERY PLAN`** on the same `SELECT` as `readOriginalDispatches`. PostgreSQL-style **`EXPLAIN ANALYZE …`** is not the SQLite shell model; extra runtime-ish detail from the CLI uses **`.timer`**, **`.scanstats`**, and the **same `SELECT`** (see `explain_read_plans.sh -A`).

---

## 1. Plans + optional shell timing / scan stats

**Goal:** Compare planner output across DBs; optionally compare **`.timer` / `.scanstats`** output from the official `sqlite3` shell for the same query.

**Status:** _in progress_

- [ ] Generated Markdown with `EXPLAIN QUERY PLAN` for all three DBs (`-a` recommended).
- [ ] (Optional) Same report with **`-A`** — runs `.timer` / `.scanstats` / same `SELECT`, but the file only includes the **trailing summary** (cycles + `Run Time`), not result rows.
- [ ] Compared Initial vs Deferred sections (and baseline if relevant).

---

## 2. Same plan shape, same objects?

**Goal:** Confirm each step uses the same tables/indexes by name (no subtle plan swap).

- [ ] Re-read the **`EXPLAIN QUERY PLAN`** output and verify index/table names match across DBs.

---

## 3. Physical DB / index shape

**Goal:** Detect size, fragmentation, or page-count differences.

- [ ] Compared file sizes, `page_count`, `freelist_count`, `page_size`.
- [ ] (Optional) Ran `sqlite3_analyzer` per file if available.
- [ ] (Optional) `VACUUM INTO` experiment if fragmentation is suspected.

---

## 4. Table vs index clustering

**Goal:** Reason about table B-tree order vs supplemental indexes (same logical index, different locality).

- [ ] Documented hypothesis after steps 1–3.

---

## 5. Benchmark methodology

**Goal:** Rule out cold vs warm cache and noise.

- [ ] Repeated runs / medians documented.
- [ ] Cold vs warm strategy noted.

---

## 6. If still unclear

**Goal:** Deeper profiling or minimal repro.

- [ ] `perf` / trace / minimal repro as needed.

---

## Reference: default DB filenames

| File | Variant |
|------|---------|
| `BM_Read_Original.db` | No supplemental indexes |
| `BM_Read_Original_Indexed_Initial.db` | Indexes before inserts |
| `BM_Read_Original_Indexed_Deferred.db` | Indexes after inserts |

Query under test: `readOriginalDispatches` in `source/schema_original.cpp`.

After changing `scripts/explain_read_plans.sh`, re-run **CMake** so the copy in your build directory updates (or invoke **`../scripts/explain_read_plans.sh`** from the build dir).
