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

Generate JSON from the benchmark app, for example:

  ./benchmark_schema --benchmark_out=results.json --benchmark_out_format=json

  ./benchmark_schema \\
      --benchmark_repetitions=5 \\
      --benchmark_out=results.json --benchmark_out_format=json
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any

WRITE_BASELINE = "BM_Write_Original"
READ_BASELINE = "BM_Read_Original"


def _repo_root() -> Path:
    """Directory containing `scripts/` (sqlite-playground project root)."""
    return Path(__file__).resolve().parent.parent


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
    h_db = _href_relative_to_md(output_md, "source/db_helpers.cpp")
    link_db = f"[`db_helpers.cpp`]({h_db})"
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
        f"| Test name | Wall time ({time_unit}) | CPU time ({time_unit}) | Δ wall vs baseline | % vs baseline |",
        "| --- | ---: | ---: | ---: | ---: |",
    ]
    for name, wall, cpu, dw, pct in rows:
        if name == baseline_name:
            lines.append(
                f"| **{name}** | **{wall:.3f}** | **{cpu:.3f}** | **{dw:+.3f}** | **{pct:+.2f}%** |"
            )
        else:
            lines.append(
                f"| {name} | {wall:.3f} | {cpu:.3f} | {dw:+.3f} | {pct:+.2f}% |"
            )
    lines.append("")
    return lines


def run_markdown(path: Path, output_md: Path | None) -> str:
    data = json.loads(path.read_text(encoding="utf-8", errors="replace"))
    benchmarks = data.get("benchmarks")
    if not isinstance(benchmarks, list):
        raise SystemExit("JSON missing 'benchmarks' array")

    stats, time_unit = _collect_wall_cpu_means(benchmarks)

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


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Build a Markdown report from Google Benchmark JSON: tables "
        "of wall/CPU means vs BM_Write_Original and BM_Read_Original baselines."
    )
    ap.add_argument(
        "json_file",
        type=Path,
        help="Path to JSON from benchmark_schema (--benchmark_out=...)",
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

    text = run_markdown(args.json_file, args.output)
    if args.output is not None:
        args.output.write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
