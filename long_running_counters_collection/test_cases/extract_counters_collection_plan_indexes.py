#!/usr/bin/env python3
"""
Regenerate proposal{2,3}_indexes_for_counters_collection.sql from bench_copies DBs.

Uses EXPLAIN QUERY PLAN SELECT * FROM counters_collection and sqlite_master.sql
for each index name found in the plan detail text.
"""
from __future__ import annotations

import re
import sqlite3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BENCH = ROOT / "bench_copies"
OUT = Path(__file__).resolve().parent

SOURCES = (
    (
        "proposal2_indexes_for_counters_collection.sql",
        BENCH / "02_proposal2_event_pmc_dispatch.db",
        "02_proposal2_event_pmc_dispatch.db",
    ),
    (
        "proposal3_indexes_for_counters_collection.sql",
        BENCH / "03_proposal3_recommended.db",
        "03_proposal3_recommended.db",
    ),
)


def plan_index_names(con: sqlite3.Connection) -> list[str]:
    cur = con.execute("EXPLAIN QUERY PLAN SELECT * FROM counters_collection")
    names: list[str] = []
    seen: set[str] = set()
    for row in cur.fetchall():
        detail = row[-1] or ""
        for m in re.finditer(r"USING (?:COVERING )?INDEX ([^ )\n]+)", detail):
            raw = m.group(1).strip('"')
            if raw.startswith("sqlite_autoindex_"):
                continue
            if raw not in seen:
                seen.add(raw)
                names.append(raw)
    return names


def index_create_sql(con: sqlite3.Connection, name: str) -> str | None:
    row = con.execute(
        "SELECT sql FROM sqlite_master WHERE type='index' AND name=?",
        (name,),
    ).fetchone()
    return row[0] if row else None


def main() -> None:
    for out_name, db_path, bench_label in SOURCES:
        if not db_path.is_file():
            raise SystemExit(f"missing {db_path}")
        con = sqlite3.connect(str(db_path))
        try:
            idxs = plan_index_names(con)
            blocks = []
            for n in idxs:
                sql = index_create_sql(con, n)
                if not sql:
                    raise SystemExit(f"no sqlite_master sql for index {n!r}")
                blocks.append(sql.strip() + ";")
            body = "\n\n".join(blocks)
            header = f"""-- Source DB: ../bench_copies/{bench_label}
-- Query: EXPLAIN QUERY PLAN SELECT * FROM counters_collection;
--
-- Named indexes referenced by the planner (non-autoindex):
-- {", ".join(idxs)}
--
-- Regenerate: python3 test_cases/extract_counters_collection_plan_indexes.py

"""
            (OUT / out_name).write_text(header + "\n" + body + "\n", encoding="utf-8")
            print(f"wrote {out_name} ({len(idxs)} indexes)")
        finally:
            con.close()


if __name__ == "__main__":
    main()
