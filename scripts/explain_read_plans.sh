#!/usr/bin/env bash
# Generate SQLite access plans for the benchmark read query (readOriginalDispatches).
# Keep the SELECT in sync with source/schema_original.cpp (readOriginalDispatches).
#
# Strategy (SQLite shell, not PostgreSQL-style EXPLAIN ANALYZE):
#   - Structural / planner: EXPLAIN QUERY PLAN + same SELECT as the benchmark.
#   - Optional extra runtime-ish detail: sqlite3 dot-commands .timer and .scanstats, then the
#     same SELECT (see -A). That is how the official sqlite3 CLI exposes timing and scan stats.
#
# Usage:
#   ./scripts/explain_read_plans.sh [options] [db1 db2 ...]
#
# With no database arguments, uses the three read-benchmark DBs from -d (default: cwd):
#   BM_Read_Original.db                  — no supplemental indexes
#   BM_Read_Original_Indexed_Initial.db  — rocpd_indexes.sql before inserts
#   BM_Read_Original_Indexed_Deferred.db — rocpd_indexes.sql after inserts
#
# Examples:
#   ./scripts/explain_read_plans.sh -a
#   ./scripts/explain_read_plans.sh -a -d ../build-s93-vm7
#   ./scripts/explain_read_plans.sh -a -A
#   ./scripts/explain_read_plans.sh path/to/A.db path/to/B.db
#
# Options:
#   -d DIR    Directory for default three DB basenames (default: .)
#   -o FILE   Write Markdown to FILE (default: ./plans/read_original_explain.md in the current directory)
#   -a        Run ANALYZE on each database before EXPLAIN (helps planner stats)
#   -A        Also append .timer + .scanstats + same SELECT (runs full query). Output is trimmed to
#             the trailing summary only (QUERY PLAN cycles / scan tree + Run Time), not result rows.

set -euo pipefail

OUT="plans/read_original_explain.md"
RUN_ANALYZE=0
RUN_SHELL_STATS=0
DEFAULT_DIR="."

while getopts "d:o:aAh" opt; do
  case "$opt" in
    d) DEFAULT_DIR="$OPTARG" ;;
    o) OUT="$OPTARG" ;;
    a) RUN_ANALYZE=1 ;;
    A) RUN_SHELL_STATS=1 ;;
    h)
      sed -n '1,45p' "$0"
      exit 0
      ;;
    *) exit 2 ;;
  esac
done
shift $((OPTIND - 1))

if [[ $# -ge 1 ]]; then
  dbs=("$@")
else
  dbs=()
  for base in BM_Read_Original.db BM_Read_Original_Indexed_Initial.db BM_Read_Original_Indexed_Deferred.db; do
    dbs+=("${DEFAULT_DIR%/}/$base")
  done
fi

if ! command -v sqlite3 >/dev/null 2>&1; then
  echo "sqlite3 not found in PATH" >&2
  exit 1
fi

mkdir -p "$(dirname "$OUT")"

any_ok=0

subtitle_for() {
  case "$1" in
    BM_Read_Original.db)
      echo "No supplemental indexes (only schema / implicit indexes)."
      ;;
    BM_Read_Original_Indexed_Initial.db)
      echo "Supplemental indexes from \`rocpd_indexes.sql\` applied **before** dispatch inserts."
      ;;
    BM_Read_Original_Indexed_Deferred.db)
      echo "Supplemental indexes from \`rocpd_indexes.sql\` applied **after** dispatch inserts."
      ;;
    *)
      echo ""
      ;;
  esac
}

explain_query_plan() {
  sqlite3 "$1" <<'SQL'
EXPLAIN QUERY PLAN
SELECT d.id, d.nid, d.pid, d.tid, d.agent_id, d.kernel_id,
       d.dispatch_id, d.queue_id, d.stream_id, d.start, d."end",
       k.kernel_name
FROM rocpd_kernel_dispatch d
JOIN rocpd_info_kernel_symbol k ON d.kernel_id = k.id
ORDER BY d.start;
SQL
}

# Official sqlite3 CLI: .timer and .scanstats, then the real SELECT. Full execution, but we strip
# leading result rows / per-row noise and keep only the trailing summary (QUERY PLAN… + Run Time).
select_with_shell_stats() {
  sqlite3 "$1" <<'SQL' 2>&1 | awk '
    /^QUERY PLAN/ { tail = 1 }
    tail { print; next }
    /^Run Time:/ { print }
  '
.timer on
.scanstats on
SELECT d.id, d.nid, d.pid, d.tid, d.agent_id, d.kernel_id,
       d.dispatch_id, d.queue_id, d.stream_id, d.start, d."end",
       k.kernel_name
FROM rocpd_kernel_dispatch d
JOIN rocpd_info_kernel_symbol k ON d.kernel_id = k.id
ORDER BY d.start;
SQL
}

{
  echo "# EXPLAIN QUERY PLAN — \`readOriginalDispatches\`"
  echo
  echo "Generated: $(date -u +%Y-%m-%dT%H:%M:%SZ) (UTC)"
  echo
  echo 'Query (must match `schema_original.cpp`):'
  echo
  echo '```sql'
  cat <<'SQLEX'
SELECT d.id, d.nid, d.pid, d.tid, d.agent_id, d.kernel_id,
       d.dispatch_id, d.queue_id, d.stream_id, d.start, d."end",
       k.kernel_name
FROM rocpd_kernel_dispatch d
JOIN rocpd_info_kernel_symbol k ON d.kernel_id = k.id
ORDER BY d.start;
SQLEX
  echo '```'
  echo

  for db in "${dbs[@]}"; do
    if [[ ! -f "$db" ]]; then
      echo "Skipping (not a file): $db" >&2
      continue
    fi
    any_ok=1
    abs="$(cd "$(dirname "$db")" && pwd)/$(basename "$db")"
    label="$(basename "$db")"
    sub="$(subtitle_for "$label")"

    echo "## ${label}"
    echo
    if [[ -n "$sub" ]]; then
      echo "$sub"
      echo
    fi
    echo "Path: \`${abs}\`"
    echo

    if [[ "$RUN_ANALYZE" -eq 1 ]]; then
      echo "Running \`ANALYZE\` …"
      sqlite3 "$db" "ANALYZE;"
      echo
    fi

    echo '### EXPLAIN QUERY PLAN'
    echo
    echo '```'
    explain_query_plan "$db"
    echo '```'
    echo

    if [[ "$RUN_SHELL_STATS" -eq 1 ]]; then
      echo '### Shell: `.timer`, `.scanstats`, and SELECT (summary only)'
      echo
      echo '```'
      select_with_shell_stats "$db"
      echo '```'
      echo
    fi
  done
} >"$OUT"

if [[ "$any_ok" -eq 0 ]]; then
  echo "Warning: no database files were processed; $OUT may only contain the query header." >&2
fi

echo "Wrote $OUT"
