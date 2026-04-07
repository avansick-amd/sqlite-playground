#!/usr/bin/env python3
"""
Analyze Google Benchmark JSON output against fixed baselines.

Baselines (original schema, no extra options in the benchmark name):
  - BM_Write_Original
  - BM_Read_Original

Mean times come from *_mean aggregate rows when present, otherwise the average
of iteration rows. **Δ** and **% vs baseline** use wall (**real**) time; the
table also lists CPU time for reference.

The Markdown report starts with a **Benchmark run summary** (repetitions,
CPU/cache info from the JSON `context` block; host name is omitted) and ends
with a **Benchmark name reference** table explaining each name suffix / variant, including a
**relative Markdown link** to `source/db_helpers.cpp` for `createDatabase` / PRAGMA context (path
computed from `-o` so it works when the report lives beside that file or under `build/`).

  ./benchmark_schema --benchmark_out=results.json --benchmark_out_format=json
  python3 analyze_benchmark_json.py results.json -o report.md
  python3 analyze_benchmark_json.py results.json   # Markdown to stdout

Read campaign (split processes, JSONL from ``read --jsonl``), same style of read table:

  python3 analyze_benchmark_json.py --read-campaign read_benchmark_dbs/read_campaign.jsonl -o read_report.md

Show raw numbers while analyzing (to stderr, so ``-o report.md`` stays clean):

  python3 analyze_benchmark_json.py --verbose --read-campaign read_campaign.jsonl -o read_report.md
  python3 analyze_benchmark_json.py --verbose results.json -o report.md
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any

WRITE_BASELINE = "BM_Write_Original"
READ_BASELINE = "BM_Read_Original"


def _repo_root() -> Path:
    """Project root (parent of `scripts/` if this file lives there, else parent of this file)."""
    d = Path(__file__).resolve().parent
    return d.parent if d.name == "scripts" else d


def _href_relative_to_md(output_md: Path | None, repo_relative: str) -> str:
    """
    Href for Markdown links, relative to the generated `.md` file’s directory.
    If `-o` is omitted (stdout), use repo-relative paths like `source/foo.cpp`.
    """
    target = (_repo_root() / repo_relative).resolve()
    if output_md is None:
        return repo_relative.replace("\\", "/")
    base = output_md.resolve().parent
    try:
        rel = os.path.relpath(target, base)
    except ValueError:
        rel = repo_relative
    return rel.replace("\\", "/")


def _extract_repetitions(benchmarks: list[dict[str, Any]]) -> int | None:
    """Google Benchmark repeats each case `repetitions` times when --benchmark_repetitions > 1."""
    for row in benchmarks:
        r = row.get("repetitions")
        if isinstance(r, int) and r > 0:
            return r
    return None


def _format_run_summary(
    data: dict[str, Any],
    benchmarks: list[dict[str, Any]],
) -> list[str]:
    """Markdown block: repetitions + environment from JSON `context` and rows (no host name)."""
    reps = _extract_repetitions(benchmarks)
    ctx = data.get("context")
    lines: list[str] = [
        "## Benchmark run summary",
        "",
    ]
    if isinstance(reps, int):
        lines.append(
            f"- **Repetitions per case:** {reps} (from `--benchmark_repetitions={reps}`)"
        )
    else:
        lines.append("- **Repetitions per case:** *not present in JSON (default 1)*")

    if not isinstance(ctx, dict):
        lines.extend(
            [
                "- **Hardware / environment:** *no `context` block in JSON*",
                "",
            ]
        )
        return lines

    exe = ctx.get("executable")
    if exe:
        lines.append(f"- **Executable:** `{exe}`")

    ncpu = ctx.get("num_cpus")
    mhz = ctx.get("mhz_per_cpu")
    scaling = ctx.get("cpu_scaling_enabled")
    if isinstance(ncpu, int) and isinstance(mhz, (int, float)):
        sc = ""
        if isinstance(scaling, bool):
            sc = f"; CPU frequency scaling: **{'on' if scaling else 'off'}**"
        mhz_f = float(mhz)
        lines.append(f"- **CPU:** {ncpu} logical CPUs @ ~{mhz_f:.0f} MHz{sc}")

    caches = ctx.get("caches")
    if isinstance(caches, list) and caches:
        parts: list[str] = []
        for c in caches:
            if not isinstance(c, dict):
                continue
            lvl = c.get("level")
            typ = c.get("type", "")
            sz = c.get("size")
            if isinstance(sz, int):
                kb = sz // 1024
                parts.append(f"L{lvl} {typ} {kb} KiB")
        if parts:
            lines.append("- **Caches:** " + "; ".join(parts))

    load = ctx.get("load_avg")
    if isinstance(load, list) and len(load) >= 1:
        la = ", ".join(f"{x:.2f}" for x in load[:3])
        lines.append(f"- **Load average (1/5/15 min):** {la}")

    dt = ctx.get("date")
    if dt:
        lines.append(f"- **JSON capture time:** {dt}")

    btype = ctx.get("library_build_type")
    if btype:
        lines.append(
            f"- **Google Benchmark library build:** `{btype}` *(Debug affects timing; prefer Release for comparisons)*"
        )

    lines.append("")
    return lines


def _glossary_section(output_md: Path | None) -> list[str]:
    """Fixed explanation of benchmark name parts + link to db_helpers for createDatabase."""
    _ = _href_relative_to_md(output_md, "source/db_helpers.cpp")
    return [
        "## Benchmark name reference",
        "",
        "Names follow `BM_<Operation>_<Schema>[_<Variant>…]`. Baselines for this report are "
        f"`{WRITE_BASELINE}` (writes) and `{READ_BASELINE}` (reads).",
        "",
        "| Part | Meaning |",
        "| --- | --- |",
        "| `Write` / `Read` | Timed operation: bulk insert + indexes vs query workload. |",
        "| `Original` | `rocpd_tables.sql` schema (original layout). |",
        "| `Combined` | `rocpd_tables_combined.sql` (deduplicated dispatch context). |",
        "| `ChunkedInserts` | Dispatches inserted with multi-row `INSERT` batches (vs one row per statement). |",
        "| `NoFKExclAccess` | **Write benchmarks only:** `PRAGMA foreign_keys = OFF` and `PRAGMA locking_mode = EXCLUSIVE` during `createDatabase` / load. Read benchmarks never use this variant; load and timed read both use FK on + NORMAL locking. |",
        "| `Indexed_Initial` | Supplemental index SQL applied **before** dispatch inserts. |",
        "| `Indexed_Deferred` | Supplemental index SQL applied **after** dispatch inserts. |",
        "| `ROCpdSourceFile` | Read directly from `benchmark_data/rocpd-7389.db` (real capture: `rocpd_kernel_dispatch_<GUID>` + join). Control vs synthetic-schema reads. |",
        "| `ROCpdSourceFile_IndexedCopy` | File copy of that DB, supplemental indexes on the real dispatch table, then same read query (indexes match `schemas/rocpd_indexes.sql` intent). |",
        "",
    ]


def _collect_wall_cpu_means(
    benchmarks: list[dict[str, Any]],
) -> tuple[dict[str, tuple[float, float]], str]:
    """
    Returns:
      stats: run_name -> (mean real_time, mean cpu_time)
      time_unit: e.g. 'ms'
    """
    iter_wall: dict[str, list[float]] = {}
    iter_cpu: dict[str, list[float]] = {}
    wall_mean: dict[str, float] = {}
    cpu_mean: dict[str, float] = {}
    time_unit = "ms"

    for row in benchmarks:
        if "time_unit" in row:
            time_unit = str(row["time_unit"])

        rt = row.get("run_type")
        name = row.get("name")
        if not name:
            continue

        if rt == "iteration":
            run_name = row.get("run_name") or name
            iter_wall.setdefault(run_name, []).append(float(row["real_time"]))
            iter_cpu.setdefault(run_name, []).append(float(row["cpu_time"]))

        elif rt == "aggregate" and name.endswith("_mean"):
            base = name[: -len("_mean")]
            wall_mean[base] = float(row["real_time"])
            cpu_mean[base] = float(row["cpu_time"])

    for run_name in set(iter_wall.keys()) | set(iter_cpu.keys()):
        if run_name not in wall_mean and iter_wall.get(run_name):
            wv = iter_wall[run_name]
            cv = iter_cpu[run_name]
            wall_mean[run_name] = sum(wv) / len(wv)
            cpu_mean[run_name] = sum(cv) / len(cv)

    names = sorted(set(wall_mean.keys()) & set(cpu_mean.keys()))
    stats = {n: (wall_mean[n], cpu_mean[n]) for n in names}
    return stats, time_unit


def _delta_pct(delta: float, baseline: float) -> float:
    if baseline == 0:
        return 0.0
    return 100.0 * delta / baseline


def _markdown_table(
    title: str,
    baseline_name: str,
    prefix: str,
    stats: dict[str, tuple[float, float]],
    time_unit: str,
    *,
    omit_cpu: bool = False,
) -> list[str]:
    names = [n for n in stats if n.startswith(prefix)]
    if baseline_name not in stats:
        return [
            f"## {title}",
            "",
            f"*No `{baseline_name}` in JSON; cannot compute.*",
            "",
        ]

    baseline_wall, _ = stats[baseline_name]
    rows: list[tuple[str, float, float, float, float]] = []
    for name in names:
        wall, cpu = stats[name]
        dw = wall - baseline_wall
        pct = _delta_pct(dw, baseline_wall)
        rows.append((name, wall, cpu, dw, pct))

    # Sort by % vs baseline (ascending: more negative / faster first)
    rows.sort(key=lambda r: r[4])

    lines: list[str] = [
        f"## {title}",
        "",
        f"Baseline: `{baseline_name}`. Δ and % use **wall (real) time** vs baseline wall time.",
        "",
    ]
    if omit_cpu:
        lines.append(
            "*CPU time was not recorded in this run (split-process read campaign); "
            "only wall time is shown.*"
        )
        lines.append("")
        lines.append(
            f"| Test name | Wall time ({time_unit}) | Δ wall vs baseline | % vs baseline |"
        )
        lines.append("| --- | ---: | ---: | ---: |")
    else:
        lines.append(
            f"| Test name | Wall time ({time_unit}) | CPU time ({time_unit}) | "
            f"Δ wall vs baseline | % vs baseline |"
        )
        lines.append("| --- | ---: | ---: | ---: | ---: |")

    for name, wall, cpu, dw, pct in rows:
        if omit_cpu:
            if name == baseline_name:
                lines.append(
                    f"| **{name}** | **{wall:.3f}** | **{dw:+.3f}** | **{pct:+.2f}%** |"
                )
            else:
                lines.append(f"| {name} | {wall:.3f} | {dw:+.3f} | {pct:+.2f}% |")
        elif name == baseline_name:
            lines.append(
                f"| **{name}** | **{wall:.3f}** | **{cpu:.3f}** | **{dw:+.3f}** | **{pct:+.2f}%** |"
            )
        else:
            lines.append(
                f"| {name} | {wall:.3f} | {cpu:.3f} | {dw:+.3f} | {pct:+.2f}% |"
            )
    lines.append("")
    return lines


def _verbose_google_benchmark(
    benchmarks: list[dict[str, Any]], time_unit: str
) -> None:
    w = sys.stderr.write
    w(f"=== Raw Google Benchmark data (units: {time_unit}) ===\n")
    by_run: dict[str, list[tuple[float, float]]] = defaultdict(list)
    aggregates: dict[str, tuple[float, float]] = {}
    for row in benchmarks:
        rt = row.get("run_type")
        name = row.get("name")
        if not name:
            continue
        if rt == "iteration":
            run_name = row.get("run_name") or str(name)
            by_run[run_name].append(
                (float(row["real_time"]), float(row["cpu_time"]))
            )
        elif rt == "aggregate" and str(name).endswith("_mean"):
            base = str(name)[: -len("_mean")]
            aggregates[base] = (float(row["real_time"]), float(row["cpu_time"]))

    for run_name in sorted(by_run.keys()):
        pairs = by_run[run_name]
        reals = [p[0] for p in pairs]
        cpus = [p[1] for p in pairs]
        w(f"\n{run_name}  (n={len(pairs)} iteration row(s))\n")
        w("  real_time: " + ", ".join(f"{x:.6f}" for x in reals) + "\n")
        w("  cpu_time:  " + ", ".join(f"{x:.6f}" for x in cpus) + "\n")
        if pairs:
            mr = sum(reals) / len(reals)
            mc = sum(cpus) / len(cpus)
            w(f"  mean real={mr:.6f}  mean cpu={mc:.6f}  {time_unit}\n")
            w(
                f"  min real={min(reals):.6f}  max real={max(reals):.6f}  {time_unit}\n"
            )

    if aggregates:
        w("\n=== Aggregate *_mean rows in JSON ===\n")
        for k in sorted(aggregates.keys()):
            wr, wc = aggregates[k]
            w(f"  {k}_mean: real={wr:.6f}  cpu={wc:.6f}  {time_unit}\n")
    w("\n")


def _verbose_read_campaign(raw: dict[str, list[float]]) -> None:
    w = sys.stderr.write
    w("=== Raw read-campaign data (wall time per process, seconds) ===\n")
    for name in sorted(raw.keys()):
        samples = raw[name]
        w(f"\n{name}  (n={len(samples)})\n")
        w("  real_time_sec: " + ", ".join(f"{s:.9f}" for s in samples) + "\n")
        mean = sum(samples) / len(samples)
        w(
            f"  mean_sec={mean:.9f}  mean_ms={1000.0 * mean:.6f}  "
            f"min_sec={min(samples):.9f}  max_sec={max(samples):.9f}\n"
        )
    w("\n")


def run_markdown(path: Path, output_md: Path | None, *, verbose: bool = False) -> str:
    data = json.loads(path.read_text(encoding="utf-8", errors="replace"))
    benchmarks = data.get("benchmarks")
    if not isinstance(benchmarks, list):
        raise SystemExit("JSON missing 'benchmarks' array")

    stats, time_unit = _collect_wall_cpu_means(benchmarks)
    if verbose:
        _verbose_google_benchmark(benchmarks, time_unit)

    lines: list[str] = [
        "# Benchmark analysis",
        "",
        f"Source: `{path}`",
        "",
    ]

    lines.extend(_format_run_summary(data, benchmarks))

    lines.extend(
        _markdown_table(
            "Write benchmarks",
            WRITE_BASELINE,
            "BM_Write",
            stats,
            time_unit,
        )
    )
    lines.extend(
        _markdown_table(
            "Read benchmarks",
            READ_BASELINE,
            "BM_Read",
            stats,
            time_unit,
        )
    )

    lines.extend(_glossary_section(output_md))

    return "\n".join(lines).rstrip() + "\n"


def _load_read_campaign_jsonl(path: Path) -> dict[str, list[float]]:
    """benchmark name -> wall times in seconds (one entry per process)."""
    by_name: dict[str, list[float]] = defaultdict(list)
    with path.open(encoding="utf-8", errors="replace") as f:
        for line_no, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            try:
                obj: Any = json.loads(line)
            except json.JSONDecodeError as e:
                raise SystemExit(f"{path}:{line_no}: invalid JSON: {e}") from e
            name = obj.get("benchmark")
            rt = obj.get("real_time_sec")
            if not isinstance(name, str) or not isinstance(rt, (int, float)):
                raise SystemExit(
                    f"{path}:{line_no}: each line must have string 'benchmark' "
                    f"and numeric 'real_time_sec'"
                )
            by_name[name].append(float(rt))
    if not by_name:
        raise SystemExit(f"{path}: no JSON lines found")
    return dict(by_name)


def _format_read_campaign_summary(
    by_name: dict[str, list[float]], path: Path
) -> list[str]:
    lines: list[str] = [
        "## Benchmark run summary",
        "",
        f"- **Source:** read campaign JSONL (`{path}`)",
        "- **Method:** one wall-clock measurement per line; each sample is a **separate** "
        "`benchmark_schema read` process (same workload as `BM_Read_*` Google Benchmark cases).",
        "",
    ]
    parts: list[str] = []
    for n in sorted(by_name.keys()):
        k = len(by_name[n])
        parts.append(f"`{n}`×{k}")
    lines.append("- **Samples:** " + ", ".join(parts))
    lines.append("")
    return lines


def run_markdown_read_campaign(
    path: Path, output_md: Path | None, *, verbose: bool = False
) -> str:
    raw = _load_read_campaign_jsonl(path)
    if verbose:
        _verbose_read_campaign(raw)
    # Mean wall time in ms (Google Benchmark-style units for the read table).
    stats: dict[str, tuple[float, float]] = {}
    for name, samples in raw.items():
        mean_ms = 1000.0 * sum(samples) / len(samples)
        stats[name] = (mean_ms, mean_ms)

    lines: list[str] = [
        "# Benchmark analysis (read campaign)",
        "",
        f"Source: `{path}`",
        "",
    ]
    lines.extend(_format_read_campaign_summary(raw, path))

    lines.extend(
        _markdown_table(
            "Read benchmarks",
            READ_BASELINE,
            "BM_Read",
            stats,
            "ms",
            omit_cpu=True,
        )
    )

    lines.append("## Write benchmarks")
    lines.append("")
    lines.append(
        "*No write timings in read-campaign JSONL; run `./benchmark_schema` with "
        "`--benchmark_out=...` to include writes.*"
    )
    lines.append("")

    lines.extend(_glossary_section(output_md))

    return "\n".join(lines).rstrip() + "\n"


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Build a Markdown report from Google Benchmark JSON: tables "
        "of wall/CPU means vs BM_Write_Original and BM_Read_Original baselines. "
        "With --read-campaign, input is JSONL from split `read --jsonl` runs."
    )
    ap.add_argument(
        "json_file",
        type=Path,
        help="Path to JSON (--benchmark_out) or JSONL (--read-campaign)",
    )
    ap.add_argument(
        "--read-campaign",
        action="store_true",
        help="Treat input as newline-delimited JSON from benchmark_schema read --jsonl",
    )
    ap.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Print raw sample times to stderr while building the report",
    )
    ap.add_argument(
        "-o",
        "--output",
        type=Path,
        metavar="FILE",
        default=None,
        help="Write Markdown report to FILE; omit to print Markdown to stdout",
    )
    args = ap.parse_args()

    if not args.json_file.is_file():
        sys.stderr.write(f"error: file not found: {args.json_file}\n")
        sys.exit(1)

    if args.read_campaign:
        text = run_markdown_read_campaign(
            args.json_file, args.output, verbose=args.verbose
        )
    else:
        text = run_markdown(args.json_file, args.output, verbose=args.verbose)
    if args.output is not None:
        args.output.write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
