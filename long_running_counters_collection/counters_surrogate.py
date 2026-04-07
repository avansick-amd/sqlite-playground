#!/usr/bin/env python3
"""
Build the counters_collection-equivalent SELECT (same joins, aggregates, subqueries)
with an optional cap on PMC_E.event_id for faster iteration runs.

Uses WHERE PMC_E.event_id <= N so EXPLAIN plans stay aligned with
SELECT * FROM counters_collection (kernel_dispatch index scan bound by event_id,
then rocpd_pmc_event via event_id), unlike a raw PMC_E.rowid cap which can
pick a different kernel_dispatch access path.

Table names are resolved from sqlite_master (GUID suffix per capture).
"""
from __future__ import annotations

import sqlite3
import time
from pathlib import Path


def discover_table_suffix(con: sqlite3.Connection) -> str:
    row = con.execute(
        """
        SELECT name FROM sqlite_master
        WHERE type='table' AND name GLOB 'rocpd_pmc_event_*'
        ORDER BY length(name) DESC LIMIT 1
        """
    ).fetchone()
    if not row:
        raise RuntimeError("no rocpd_pmc_event_* table in database")
    name = row[0]
    prefix = "rocpd_pmc_event_"
    if not name.startswith(prefix):
        raise RuntimeError(f"unexpected pmc_event table name: {name}")
    return name[len(prefix) :]


def rocpd_table(suffix: str, stem: str) -> str:
    return f"rocpd_{stem}_{suffix}"


def surrogate_select_sql(suffix: str, max_event_id: int | None) -> str:
    pmc = rocpd_table(suffix, "pmc_event")
    pmc_i = rocpd_table(suffix, "info_pmc")
    ev = rocpd_table(suffix, "event")
    kd = rocpd_table(suffix, "kernel_dispatch")
    ag = rocpd_table(suffix, "info_agent")
    sym = rocpd_table(suffix, "info_kernel_symbol")
    proc = rocpd_table(suffix, "info_process")
    thr = rocpd_table(suffix, "info_thread")
    rstr = rocpd_table(suffix, "string")

    where_event = ""
    if max_event_id is not None:
        where_event = f"\nWHERE PMC_E.event_id <= {int(max_event_id)}"

    return f"""
SELECT
    MIN(PMC_E.id) AS id,
    PMC_E.guid,
    K.dispatch_id,
    K.kernel_id,
    E.id AS event_id,
    E.correlation_id,
    E.stack_id,
    E.parent_stack_id,
    P.pid,
    T.tid,
    K.agent_id,
    A.absolute_index AS agent_abs_index,
    A.logical_index AS agent_log_index,
    A.type_index AS agent_type_index,
    A.type AS agent_type,
    K.queue_id,
    K.grid_size_x AS grid_size_x,
    K.grid_size_y AS grid_size_y,
    K.grid_size_z AS grid_size_z,
    (K.grid_size_x * K.grid_size_y * K.grid_size_z) AS grid_size,
    S.display_name AS kernel_name,
    (
        SELECT string FROM `{rstr}` RS
        WHERE RS.id = K.region_name_id AND RS.guid = K.guid
    ) AS kernel_region,
    K.workgroup_size_x AS workgroup_size_x,
    K.workgroup_size_y AS workgroup_size_y,
    K.workgroup_size_z AS workgroup_size_z,
    (K.workgroup_size_x * K.workgroup_size_y * K.workgroup_size_z) AS workgroup_size,
    K.group_segment_size AS lds_block_size,
    K.private_segment_size AS scratch_size,
    S.arch_vgpr_count AS vgpr_count,
    S.accum_vgpr_count,
    S.sgpr_count,
    PMC_I.name AS counter_name,
    PMC_I.symbol AS counter_symbol,
    PMC_I.component,
    PMC_I.description,
    PMC_I.block,
    PMC_I.expression,
    PMC_I.value_type,
    PMC_I.id AS counter_id,
    SUM(PMC_E.value) AS value,
    K.start,
    K.end,
    PMC_I.is_constant,
    PMC_I.is_derived,
    (K.end - K.start) AS duration,
    (
        SELECT string FROM `{rstr}` RS
        WHERE RS.id = E.category_id AND RS.guid = E.guid
    ) AS category,
    K.nid,
    E.extdata,
    S.code_object_id
FROM `{pmc}` PMC_E
INNER JOIN `{pmc_i}` PMC_I ON PMC_I.id = PMC_E.pmc_id AND PMC_I.guid = PMC_E.guid
INNER JOIN `{ev}` E ON E.id = PMC_E.event_id AND E.guid = PMC_E.guid
INNER JOIN `{kd}` K ON K.event_id = PMC_E.event_id AND K.guid = PMC_E.guid
INNER JOIN `{ag}` A ON A.id = K.agent_id AND A.guid = K.guid
INNER JOIN `{sym}` S ON S.id = K.kernel_id AND S.guid = K.guid
INNER JOIN `{proc}` P ON P.id = K.pid AND P.guid = K.guid
INNER JOIN `{thr}` T ON T.id = K.tid AND T.guid = K.guid
{where_event}
GROUP BY
    PMC_E.guid,
    K.dispatch_id,
    PMC_I.name,
    K.agent_id
""".strip()


def max_rocpd_event_id(con: sqlite3.Connection, suffix: str) -> int:
    ev = rocpd_table(suffix, "event")
    row = con.execute(f"SELECT MAX(id) FROM `{ev}`").fetchone()
    if not row or row[0] is None:
        return 0
    return int(row[0])


def open_suffix(db_path: Path) -> tuple[sqlite3.Connection, str]:
    con = sqlite3.connect(str(db_path))
    suf = discover_table_suffix(con)
    return con, suf


def time_surrogate_fetch(
    con: sqlite3.Connection,
    suffix: str,
    max_event_id: int | None,
    fetch_many: int,
) -> tuple[int, float]:
    """Execute surrogate SELECT; drain with fetchmany. Returns (row_count, seconds)."""
    sql = surrogate_select_sql(suffix, max_event_id)
    t0 = time.perf_counter()
    cur = con.execute(sql)
    n = 0
    while True:
        chunk = cur.fetchmany(fetch_many)
        if not chunk:
            break
        n += len(chunk)
    return n, time.perf_counter() - t0
