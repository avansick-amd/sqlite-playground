#!/usr/bin/env bash
# Run split read benchmarking: one process prepares six DBs; each read runs in its own process.
# Outer loop: iterations (default 50). Inner loop: all six read flavours in a fixed order.
#
# Environment (optional):
#   BENCHMARK_SCHEMA   path to benchmark_schema (default: ./benchmark_schema next to this script)
#   READ_DB_DIR        directory for persisted .db files (default: read_benchmark_dbs)
#   READ_ITERATIONS    number of full passes over the six reads (default: 50)
#   READ_JSONL         append-only timing log (default: read_campaign.jsonl in READ_DB_DIR)
#   READ_SKIP_PREPARE  if set to 1, skip prepare-read-dbs (use after a separate prepare run)
#
# By default this script runs prepare-read-dbs once at the start, then all read processes.
# To prepare in a separate execution only:
#   ./benchmark_schema prepare-read-dbs --db-dir read_benchmark_dbs
#   READ_SKIP_PREPARE=1 READ_ITERATIONS=50 ./run_read_campaign.sh
#
# Usage:
#   cd build && ./run_read_campaign.sh
#   READ_ITERATIONS=100 ./run_read_campaign.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BIN="${BENCHMARK_SCHEMA:-$SCRIPT_DIR/benchmark_schema}"
DBDIR="${READ_DB_DIR:-read_benchmark_dbs}"
ITERS="${READ_ITERATIONS:-50}"
JSONL="${READ_JSONL:-$DBDIR/read_campaign.jsonl}"

READ_CASES=(
  BM_Read_Original
  BM_Read_Original_Indexed_Initial
  BM_Read_Original_Indexed_Deferred
  BM_Read_Combined
  BM_Read_Combined_Indexed_Initial
  BM_Read_Combined_Indexed_Deferred
)

mkdir -p "$DBDIR"
if [[ "${READ_SKIP_PREPARE:-0}" == "1" ]]; then
  echo "Skipping prepare-read-dbs (READ_SKIP_PREPARE=1); using existing DBs under $DBDIR" >&2
else
  "$BIN" prepare-read-dbs --db-dir "$DBDIR"
fi

# No extra delay here — the gap until "Wrote timings" is this loop (each read is a new process).
echo "Running ${ITERS} iteration(s) × ${#READ_CASES[@]} reads (no output per read unless the binary prints)..." >&2

for ((i = 1; i <= ITERS; i++)); do
  echo "  iteration ${i}/${ITERS} ..." >&2
  for case in "${READ_CASES[@]}"; do
    "$BIN" read --case "$case" --db-dir "$DBDIR" --iteration "$i" --jsonl "$JSONL"
  done
done

echo "Wrote timings to $JSONL ($ITERS iteration(s) × ${#READ_CASES[@]} reads)."
echo "Markdown report (same style as Google Benchmark analysis):"
echo "  python3 analyze_benchmark_json.py --read-campaign \"$JSONL\" -o read_report.md"
echo "  python3 analyze_benchmark_json.py -v --read-campaign \"$JSONL\" -o read_report.md   # raw numbers on stderr"
