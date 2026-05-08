#!/usr/bin/env bash
# kagura-flags.sh
# Emits the clang flags for kagura obfuscation based on KAGURA_* env vars.
# Called from Xcode via OTHER_CFLAGS = $(shell ...).
#
# Can also be sourced directly:
#   export $(cat kagura.xcconfig | grep ^KAGURA | xargs)
#   FLAGS=$(./kagura-flags.sh)

set -euo pipefail

PLUGIN="${KAGURA_PLUGIN_PATH:-}"
if [ -z "$PLUGIN" ] || [ ! -f "$PLUGIN" ]; then
  # Try to find the plugin relative to this script
  SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
  PLUGIN="$SCRIPT_DIR/../../build/lib/Transforms/KaguraObfuscator.dylib"
fi

if [ ! -f "$PLUGIN" ]; then
  echo "# kagura: plugin not found at $PLUGIN" >&2
  exit 0
fi

FLAGS="-fpass-plugin=$PLUGIN"

add_flag() {
  FLAGS="$FLAGS -mllvm $1"
}

[ "${KAGURA_ENABLE_STR:-1}"     = "1" ] && add_flag "-kagura-str"
[ "${KAGURA_ENABLE_FLA:-1}"     = "1" ] && add_flag "-kagura-fla"
[ "${KAGURA_ENABLE_BCF:-1}"     = "1" ] && add_flag "-kagura-bcf"
[ "${KAGURA_ENABLE_SUB:-1}"     = "1" ] && add_flag "-kagura-sub"
[ "${KAGURA_ENABLE_CO:-0}"      = "1" ] && add_flag "-kagura-co"
[ "${KAGURA_ENABLE_OBJC:-1}"    = "1" ] && add_flag "-kagura-objc"
[ "${KAGURA_ENABLE_ANTIDEB:-1}" = "1" ] && add_flag "-kagura-anti-debug"
[ "${KAGURA_ENABLE_METRICS:-0}" = "1" ] && add_flag "-kagura-metrics"

[ -n "${KAGURA_BCF_PROB:-}" ] && add_flag "-kagura-bcf-prob=${KAGURA_BCF_PROB}"
[ -n "${KAGURA_BCF_ITER:-}" ] && add_flag "-kagura-bcf-iter=${KAGURA_BCF_ITER}"
[ -n "${KAGURA_SUB_ITER:-}" ] && add_flag "-kagura-sub-iter=${KAGURA_SUB_ITER}"
[ -n "${KAGURA_SEED:-}" ]     && add_flag "-kagura-seed=${KAGURA_SEED}"

echo "$FLAGS"
