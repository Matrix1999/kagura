#!/bin/bash
# add_crash.sh — add a new crash reproducer to the regression corpus.
# Usage: ./add_crash.sh <input.ll> "Description of the crash" [pass-pipeline]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CORPUS_DIR="$SCRIPT_DIR/../corpus"

INPUT="$1"
DESCRIPTION="${2:-}"
PASS="${3:-function(kagura-fla,kagura-sub,kagura-bcf)}"

# Find the next crash ID
NEXT_ID=$(ls "$CORPUS_DIR"/crash_*.ll 2>/dev/null | wc -l)
NEXT_ID=$(printf "%05d" $((NEXT_ID + 1)))
BASE="crash_${NEXT_ID}"

cp "$INPUT" "$CORPUS_DIR/${BASE}.ll"
cat > "$CORPUS_DIR/${BASE}.meta" <<EOF
PASS=${PASS}
DESCRIPTION=${DESCRIPTION}
EOF

echo "Added: ${BASE}.ll"
echo "  Pass: ${PASS}"
echo "  Description: ${DESCRIPTION}"
