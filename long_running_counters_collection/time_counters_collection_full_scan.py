#!/usr/bin/env python3
"""
Time a full read of counters_collection: execute SELECT * and drain with fetchmany
(constant memory). Does not materialize all rows in one Python list.
"""
from __future__ import annotations

import argparse
import sqlite3
import sys
import time
from pathlib import Path


def full_scan_seconds(db_path: Path, fetch_many: int) -> tuple[int, float]:
    con = sqlite3.connect(db_path)
    cur = con.cursor()
    t0 = time.perf_counter()
    cur.execute("SELECT * FROM counters_collection")
    n = 0
    while True:
        chunk = cur.fetchmany(fetch_many)
        if not chunk:
            break
        n += len(chunk)
    elapsed = time.perf_counter() - t0
    con.close()
    return n, elapsed


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("dbs", nargs="+", type=Path, help="SQLite files to scan")
    p.add_argument(
        "--fetch-many",
        type=int,
        default=8192,
        help="fetchmany() batch size (default 8192)",
    )
    args = p.parse_args()
    print("Full scan: SELECT * FROM counters_collection (rows discarded after read)")
    print(f"fetchmany size: {args.fetch_many}\n")
    for path in args.dbs:
        if not path.is_file():
            print(f"{path}: MISSING", file=sys.stderr)
            continue
        n, sec = full_scan_seconds(path, args.fetch_many)
        rps = n / sec if sec > 0 else 0.0
        print(f"{path.name}")
        print(f"  rows: {n:,}")
        print(f"  time: {sec:.3f} s")
        print(f"  rate: {rps:,.0f} rows/s\n")


if __name__ == "__main__":
    main()
