#!/usr/bin/env bash
# Compare Indexed-Initial vs Indexed-Deferred databases: physical shape + read-path cost.
# Use two .db files produced by the same benchmark order mode (e.g. both OrderById).
#
# What this does NOT fix by itself: it surfaces *where* time/pages differ (.timer, .scanstats,
# page_count, freelist). Interpreting “why” is in plans/READ_QUERY_INVESTIGATION_CHECKLIST.md.
#
# Follow-ups if deferred is slower:
#   - Re-run with -a if you forgot ANALYZE in the benchmark path.
#   - On a *copy* of the deferred DB: REINDEX; or VACUUM INTO clean.db; then re-time reads.
#   - If freelist_count or file size is much larger on deferred, fragmentation may dominate.
#
# Usage:
#   ./compare_initial_deferred_read.sh [-s combined|original] [-a] INITIAL.db DEFERRED.db
#
# Examples:
#   ./compare_initial_deferred_read.sh -a \\
#     BM_Read_Combined_Indexed_Initial_OrderById.db \\
#     BM_Read_Combined_Indexed_Deferred_OrderById.db
#
#   ./compare_initial_deferred_read.sh -s original -a ../build/BM_Read_Original_Indexed_Initial.db ../build/BM_Read_Original_Indexed_Deferred.db

set -euo pipefail

SCHEMA="combined"
RUN_ANALYZE=0

while getopts "s:ah" opt; do
  case "$opt" in
    s) SCHEMA="$OPTARG" ;;
    a) RUN_ANALYZE=1 ;;
    h)
      sed -n '1,35p' "$0"
      exit 0
      ;;
    *) exit 2 ;;
  esac
done
shift $((OPTIND - 1))

if [[ $# -ne 2 ]]; then
  echo "usage: $0 [-s combined|original] [-a] INITIAL.db DEFERRED.db" >&2
  exit 2
fi

DB_INITIAL="$1"
DB_DEFERRED="$2"

for db in "$DB_INITIAL" "$DB_DEFERRED"; do
  if [[ ! -f "$db" ]]; then
    echo "error: not a file: $db" >&2
    exit 1
  fi
done

if ! command -v sqlite3 >/dev/null 2>&1; then
  echo "sqlite3 not found in PATH" >&2
  exit 1
fi

case "$SCHEMA" in
  combined)
    READ_SQL=$(cat <<'SQL'
SELECT d.id, c.nid, c.pid, c.tid, c.agent_id, d.kernel_id,
       d.dispatch_id, c.queue_id, c.stream_id, d.start, d."end",
       k.kernel_name
FROM rocpd_kernel_dispatch d
JOIN rocpd_dispatch_context c ON d.context_id = c.id
JOIN rocpd_info_kernel_symbol k ON d.kernel_id = k.id
ORDER BY d.start;
SQL
)
    ;;
  original)
    READ_SQL=$(cat <<'SQL'
SELECT d.id, d.nid, d.pid, d.tid, d.agent_id, d.kernel_id,
       d.dispatch_id, d.queue_id, d.stream_id, d.start, d."end",
       k.kernel_name
FROM rocpd_kernel_dispatch d
JOIN rocpd_info_kernel_symbol k ON d.kernel_id = k.id
ORDER BY d.start;
SQL
)
    ;;
  *)
    echo "error: -s must be combined or original" >&2
    exit 2
    ;;
esac

physical_report() {
  local db="$1"
  local label="$2"
  echo "=== ${label} ==="
  echo "path: $(cd "$(dirname "$db")" && pwd)/$(basename "$db")"
  echo "bytes: $(wc -c <"$db")"
  sqlite3 "$db" <<SQL
PRAGMA page_size;
PRAGMA page_count;
PRAGMA freelist_count;
PRAGMA schema_version;
SQL
  echo "indexes (user):"
  sqlite3 "$db" "SELECT name FROM sqlite_master WHERE type='index' AND sql IS NOT NULL ORDER BY 1;"
  echo
}

# Same trimming idea as explain_read_plans.sh: result rows first, then QUERY PLAN / Run Time.
scan_read() {
  local db="$1"
  local label="$2"
  echo "=== ${label}: .timer + .scanstats + read query (summary: plan + Run Time) ==="
  if [[ "$RUN_ANALYZE" -eq 1 ]]; then
    sqlite3 "$db" "ANALYZE;"
  fi
  sqlite3 "$db" 2>&1 <<SQL | awk '
    /^QUERY PLAN/ { tail = 1 }
    tail { print; next }
    /^Run Time:/ { print }
  '
.timer on
.scanstats on
${READ_SQL}
SQL
  echo
}

echo "Schema mode: ${SCHEMA} (must match how the .db files were built)"
echo

physical_report "$DB_INITIAL" "Indexed-Initial"
physical_report "$DB_DEFERRED" "Indexed-Deferred"

echo "---------- Read path (compare Run Time and scan tree lines) ----------"
scan_read "$DB_INITIAL" "Indexed-Initial"
scan_read "$DB_DEFERRED" "Indexed-Deferred"

echo "Done. If plans match but Run Time differs, compare freelist/page_count and scanstats seek/page counts."
