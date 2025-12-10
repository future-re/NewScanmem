#!/usr/bin/env bash
set -euo pipefail

# copy_coverage_html.sh
# Copy generated coverage HTML into mkdocs docs so the full report can be
# served/embedded by MkDocs. Run this before `mkdocs build` or `mkdocs serve`.

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC_DIR="$REPO_ROOT/build/coverage/html"
DEST_DIR="$REPO_ROOT/mkdocs/docs/coverage_html"

if [ ! -d "$SRC_DIR" ]; then
  echo "Error: coverage HTML directory not found: $SRC_DIR"
  exit 1
fi

rm -rf "$DEST_DIR"
mkdir -p "$DEST_DIR"
cp -a "$SRC_DIR"/. "$DEST_DIR/"

echo "Copied coverage HTML from $SRC_DIR -> $DEST_DIR"
