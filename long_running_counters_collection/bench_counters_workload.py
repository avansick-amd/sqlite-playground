#!/usr/bin/env python3
"""
Bench tooling for counters_collection index proposals.

  setup          Copy source SQLite into bench_copies/01_*, 02_*, 03_* and CREATE INDEX on proposals.
  calibrate      Pick max_event_id so surrogate run time is near --target-seconds (control DB).
  run-surrogate  Time the surrogate on each DB in a fresh process per run (spawn); N passes; then CSV + summary.
  explain        EXPLAIN QUERY PLAN for surrogate and/or SELECT * FROM counters_collection.

Surrogate uses WHERE PMC_E.event_id <= N so plans stay aligned with the real view
(kernel_dispatch index bound by event_id, not a rowid-only cap on pmc_event).
"""
from __future__ import annotations

import argparse
import csv
import multiprocessing
import shutil
import sqlite3
import statistics
import sys
import time
import traceback
from collections import defaultdict
from pathlib import Path

import counters_surrogate as cs

ROOT = Path(__file__).resolve().parent
BENCH = ROOT / "bench_copies"
DEFAULT_SOURCE = ROOT / "SQC_DCACHE_INFLIGHT_LEVEL_20548.db"
PROPOSAL2_SQL = ROOT / "test_cases" / "proposal2_indexes_for_counters_collection.sql"
PROPOSAL3_SQL = ROOT / "test_cases" / "proposal3_indexes_for_counters_collection.sql"

OUT_CONTROL = BENCH / "01_control.db"
OUT_P2 = BENCH / "02_proposal2_event_pmc_dispatch.db"
OUT_P3 = BENCH / "03_proposal3_recommended.db"


def load_executable_statements(sql_path: Path) -> list[str]:
    text = sql_path.read_text(encoding="utf-8")
    lines = [ln for ln in text.splitlines() if not ln.strip().startswith("--")]
    buf = "\n".join(lines)
    out: list[str] = []
    for part in buf.split(";"):
        p = part.strip()
        if p:
            out.append(p + ";")
    return out


def apply_indexes(db_path: Path, proposal_sql: Path) -> None:
    stmts = load_executable_statements(proposal_sql)
    con = sqlite3.connect(str(db_path))
    try:
        for stmt in stmts:
            con.execute(stmt)
        con.commit()
    finally:
        con.close()


def cmd_setup(args: argparse.Namespace) -> None:
    src = Path(args.source).resolve()
    if not src.is_file():
        raise SystemExit(f"source database not found: {src}")
    BENCH.mkdir(parents=True, exist_ok=True)
    print(f"Copy (1/3) {src.name} -> {OUT_CONTROL.name} …")
    shutil.copy2(src, OUT_CONTROL)
    print(f"Copy (2/3) {OUT_CONTROL.name} -> {OUT_P2.name} …")
    shutil.copy2(OUT_CONTROL, OUT_P2)
    print(f"Copy (3/3) {OUT_CONTROL.name} -> {OUT_P3.name} …")
    shutil.copy2(OUT_CONTROL, OUT_P3)
    if not PROPOSAL2_SQL.is_file() or not PROPOSAL3_SQL.is_file():
        raise SystemExit("missing test_cases/proposal{2,3}_indexes_for_counters_collection.sql")
    print(f"Indexes on {OUT_P2.name} from {PROPOSAL2_SQL.name} …")
    apply_indexes(OUT_P2, PROPOSAL2_SQL)
    print(f"Indexes on {OUT_P3.name} from {PROPOSAL3_SQL.name} …")
    apply_indexes(OUT_P3, PROPOSAL3_SQL)
    print("Done. Regenerate plan snippets with: python3 test_cases/extract_counters_collection_plan_indexes.py")


def cmd_calibrate(args: argparse.Namespace) -> None:
    db = Path(args.db).resolve()
    if not db.is_file():
        raise SystemExit(f"database not found: {db}")
    target = float(args.target_seconds)
    fetch_many = int(args.fetch_many)
    con = sqlite3.connect(str(db))
    try:
        suffix = cs.discover_table_suffix(con)
        hi_nat = cs.max_rocpd_event_id(con, suffix)
        if hi_nat <= 0:
            raise SystemExit("empty rocpd_event max(id)")
        e_lo, t_lo = 0, 0.0
        e = max(1, args.seed_start)
        e_hi, t_hi = hi_nat, -1.0
        while True:
            n, t = cs.time_surrogate_fetch(con, suffix, e, fetch_many)
            print(f"  probe max_event_id={e}: {n:,} rows in {t:.3f} s")
            if t >= target:
                e_hi, t_hi = e, t
                break
            e_lo, t_lo = e, t
            if e >= hi_nat:
                print(
                    f"Full capture (max_event_id={hi_nat}) still under {target} s; "
                    f"use --max-event-id {hi_nat} or lower --target-seconds."
                )
                return
            nxt = e * 2 if e < 2000 else int(e * 1.5)
            e = min(max(nxt, e + 1), hi_nat)
            if e == e_lo:
                e = min(e_lo + 1, hi_nat)
            if e <= e_lo:
                raise SystemExit("calibration bracket failed; try --seed-start")
        if e_lo == 0:
            e_lo = 1
            _, t_lo = cs.time_surrogate_fetch(con, suffix, e_lo, fetch_many)
        if t_hi <= t_lo:
            guess = e_hi
        else:
            frac = (target - t_lo) / (t_hi - t_lo)
            guess = int(e_lo + (e_hi - e_lo) * frac)
        guess = max(e_lo + 1, min(guess, e_hi))
        n_g, t_g = cs.time_surrogate_fetch(con, suffix, guess, fetch_many)
        print(
            f"Interpolated max_event_id={guess} (bracket {e_lo}..{e_hi}): "
            f"{n_g:,} rows in {t_g:.3f} s"
        )
        print(f"Suggested: --max-event-id {guess}")
    finally:
        con.close()


def _surrogate_child_main(
    result_q: multiprocessing.Queue,
    db_path_str: str,
    max_event_id: int | None,
    fetch_many: int,
) -> None:
    """Runs in a fresh process: open DB, run surrogate once, put result on queue."""
    try:
        path = Path(db_path_str)
        con = sqlite3.connect(str(path))
        try:
            suffix = cs.discover_table_suffix(con)
            me = max_event_id if max_event_id is not None else cs.max_rocpd_event_id(con, suffix)
            n, sec = cs.time_surrogate_fetch(con, suffix, me, fetch_many)
            result_q.put(("ok", path.name, me, n, sec))
        finally:
            con.close()
    except Exception:
        result_q.put(("err", traceback.format_exc()))


def _run_surrogate_in_fresh_process(
    ctx: multiprocessing.context.BaseContext,
    db_path: Path,
    max_event_id: int | None,
    fetch_many: int,
) -> tuple[str, int, int, float]:
    """Start child with spawn, return (db_name, max_event_id_used, rows, seconds)."""
    q = ctx.Queue()
    p = ctx.Process(
        target=_surrogate_child_main,
        args=(q, str(db_path.resolve()), max_event_id, fetch_many),
    )
    p.start()
    p.join()
    if p.exitcode != 0:
        err_detail = ""
        try:
            msg = q.get_nowait()
            if msg[0] == "err":
                err_detail = msg[1]
        except Exception:
            pass
        raise RuntimeError(
            f"surrogate child exited with code {p.exitcode}"
            + (f":\n{err_detail}" if err_detail else "")
        )
    msg = q.get()
    if msg[0] == "err":
        raise RuntimeError(f"surrogate worker failed:\n{msg[1]}")
    _ok, db_name, me, n, sec = msg
    return db_name, me, n, sec


def _print_surrogate_summary(
    recorded: list[tuple[str, int, int, int, float]],
) -> None:
    """recorded: (db_name, pass_no, max_event_id, row_count, seconds)."""
    by_db_times: dict[str, list[float]] = defaultdict(list)
    by_db_rows: dict[str, set[int]] = defaultdict(set)
    by_db_me: dict[str, set[int]] = defaultdict(set)
    for db_name, _pass_no, me, n, sec in recorded:
        by_db_times[db_name].append(sec)
        by_db_rows[db_name].add(n)
        by_db_me[db_name].add(me)

    print("", file=sys.stderr)
    print("=== Surrogate summary (all passes) ===", file=sys.stderr)
    for db_name in sorted(by_db_times.keys()):
        times = by_db_times[db_name]
        n_pass = len(times)
        mean_t = statistics.mean(times)
        st = statistics.stdev(times) if n_pass > 1 else 0.0
        print(
            f"{db_name}:  passes={n_pass}  "
            f"mean={mean_t:.3f}s  "
            f"stdev={st:.3f}s  "
            f"min={min(times):.3f}s  max={max(times):.3f}s",
            file=sys.stderr,
        )
        if len(by_db_rows[db_name]) > 1:
            print(
                f"  warning: row counts differ across runs: {sorted(by_db_rows[db_name])}",
                file=sys.stderr,
            )
        if len(by_db_me[db_name]) > 1:
            print(
                f"  warning: max_event_id values differ: {sorted(by_db_me[db_name])}",
                file=sys.stderr,
            )


def cmd_run_surrogate(args: argparse.Namespace) -> None:
    fetch_many = int(args.fetch_many)
    max_event_id = args.max_event_id
    n_passes = int(args.iterations)
    in_process = bool(args.in_process)

    resolved: list[Path] = []
    for dbp in args.dbs:
        path = Path(dbp).resolve()
        if not path.is_file():
            print(f"missing {path}", file=sys.stderr)
            continue
        resolved.append(path)

    if not resolved:
        raise SystemExit("no valid database files")

    ctx = multiprocessing.get_context("spawn")
    connections: list[tuple[Path, sqlite3.Connection, str, int]] = []
    if in_process:
        for path in resolved:
            con = sqlite3.connect(str(path))
            suffix = cs.discover_table_suffix(con)
            me = max_event_id if max_event_id is not None else cs.max_rocpd_event_id(con, suffix)
            connections.append((path, con, suffix, me))

    recorded: list[tuple[str, int, int, int, float]] = []
    n_dbs = len(resolved)
    try:
        for pass_no in range(1, n_passes + 1):
            for idx, path in enumerate(resolved):
                db_i = idx + 1
                if not args.no_progress:
                    print(
                        f"starting surrogate: iteration {pass_no}/{n_passes}, "
                        f"database {db_i}/{n_dbs}: {path.name}",
                        file=sys.stderr,
                        flush=True,
                    )
                if in_process:
                    path_c, con, suffix, me = connections[idx]
                    assert path_c == path
                    n, sec = cs.time_surrogate_fetch(con, suffix, me, fetch_many)
                    db_name, me_used = path.name, me
                else:
                    db_name, me_used, n, sec = _run_surrogate_in_fresh_process(
                        ctx, path, max_event_id, fetch_many
                    )
                if not args.no_progress:
                    print(
                        f"finished surrogate: iteration {pass_no}/{n_passes}, "
                        f"database {db_i}/{n_dbs}: {path.name} "
                        f"({n:,} rows, {sec:.3f}s)",
                        file=sys.stderr,
                        flush=True,
                    )
                recorded.append((db_name, pass_no, me_used, n, sec))
    finally:
        for _path, con, _suffix, _me in connections:
            con.close()

    writer = csv.writer(sys.stdout, lineterminator="\n")
    writer.writerow(["db", "iteration", "max_event_id", "rows", "seconds"])
    for db_name, pass_no, me, n, sec in recorded:
        writer.writerow([db_name, pass_no, me, n, f"{sec:.6f}"])
    sys.stdout.flush()

    if not args.no_summary:
        _print_surrogate_summary(recorded)


def cmd_explain(args: argparse.Namespace) -> None:
    db = Path(args.db).resolve()
    if not db.is_file():
        raise SystemExit(f"database not found: {db}")
    con = sqlite3.connect(str(db))
    try:
        suffix = cs.discover_table_suffix(con)
        me = args.max_event_id
        if args.surrogate:
            sql = cs.surrogate_select_sql(suffix, me)
            print("=== EXPLAIN QUERY PLAN surrogate ===")
            for row in con.execute(f"EXPLAIN QUERY PLAN {sql}").fetchall():
                print(row[-1])
        if args.view:
            print("=== EXPLAIN QUERY PLAN SELECT * FROM counters_collection ===")
            for row in con.execute(
                "EXPLAIN QUERY PLAN SELECT * FROM counters_collection"
            ).fetchall():
                print(row[-1])
    finally:
        con.close()


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("setup", help="Rebuild bench_copies from source + proposal indexes")
    s.add_argument(
        "--source",
        type=Path,
        default=DEFAULT_SOURCE,
        help=f"path to pristine DB (default: {DEFAULT_SOURCE.name})",
    )
    s.set_defaults(func=cmd_setup)

    c = sub.add_parser("calibrate", help="Find max_event_id near target wall time (surrogate)")
    c.add_argument("--db", type=Path, default=OUT_CONTROL, help="control database")
    c.add_argument(
        "--target-seconds",
        type=float,
        default=60.0,
        help="desired surrogate duration (default: 60)",
    )
    c.add_argument("--fetch-many", type=int, default=8192)
    c.add_argument(
        "--seed-start",
        type=int,
        default=500,
        help="first max_event_id probe when bracketing (default: 500)",
    )
    c.set_defaults(func=cmd_calibrate)

    r = sub.add_parser("run-surrogate", help="CSV timings for surrogate on DB list")
    r.add_argument(
        "dbs",
        nargs="+",
        type=Path,
        help="sqlite files (e.g. bench_copies/01_control.db …)",
    )
    r.add_argument(
        "--max-event-id",
        type=int,
        default=None,
        help="cap PMC_E.event_id (default: full capture max id)",
    )
    r.add_argument(
        "--iterations",
        type=int,
        default=1,
        metavar="N",
        help="number of full passes over the DB list (pass p = run each DB once, in order; default: 1)",
    )
    r.add_argument("--fetch-many", type=int, default=8192)
    r.add_argument(
        "--no-progress",
        action="store_true",
        help="suppress starting/finished query lines on stderr",
    )
    r.add_argument(
        "--no-summary",
        action="store_true",
        help="suppress per-DB mean/stdev/min/max summary on stderr after CSV",
    )
    r.add_argument(
        "--in-process",
        action="store_true",
        help="reuse parent process and DB connections (faster; not isolated)",
    )
    r.set_defaults(func=cmd_run_surrogate)

    e = sub.add_parser("explain", help="Print EXPLAIN QUERY PLAN")
    e.add_argument("--db", type=Path, required=True)
    e.add_argument(
        "--max-event-id",
        type=int,
        default=None,
        help="for surrogate plan (default: full capture)",
    )
    e.add_argument("--surrogate", action="store_true", help="print surrogate plan")
    e.add_argument("--view", action="store_true", help="print view plan")
    e.set_defaults(func=cmd_explain)

    args = p.parse_args()
    if args.cmd == "explain" and not args.surrogate and not args.view:
        args.surrogate = True
        args.view = True
    if args.cmd == "explain" and args.max_event_id is None and args.surrogate:
        con = sqlite3.connect(str(Path(args.db).resolve()))
        try:
            suf = cs.discover_table_suffix(con)
            args.max_event_id = cs.max_rocpd_event_id(con, suf)
        finally:
            con.close()
    args.func(args)


if __name__ == "__main__":
    main()
