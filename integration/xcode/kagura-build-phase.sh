#!/usr/bin/env bash
# kagura-build-phase.sh
# Xcode "Run Script" build phase — validates the kagura plugin is present
# and prints the active obfuscation configuration to the build log.
#
# Add this as a Run Script phase BEFORE the Compile Sources phase:
#   Shell: /bin/bash
#   Script: ${SRCROOT}/../kagura/integration/xcode/kagura-build-phase.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PLUGIN="${KAGURA_PLUGIN_PATH:-$SCRIPT_DIR/../../build/lib/Transforms/KaguraObfuscator.dylib}"

echo "════════════════════════════════════════"
echo "  kagura obfuscation configuration"
echo "════════════════════════════════════════"
echo "  Plugin: $PLUGIN"

if [ ! -f "$PLUGIN" ]; then
  echo "  ⚠️  Plugin not found — obfuscation DISABLED"
  echo "  Build kagura first: cd $(dirname $SCRIPT_DIR) && bash build.sh"
  echo "════════════════════════════════════════"
  exit 0
fi

echo "  String encryption : ${KAGURA_ENABLE_STR:-1}"
echo "  CFG flattening    : ${KAGURA_ENABLE_FLA:-1}"
echo "  Bogus control flow: ${KAGURA_ENABLE_BCF:-1}  (prob=${KAGURA_BCF_PROB:-30}%)"
echo "  Substitution      : ${KAGURA_ENABLE_SUB:-1}  (iter=${KAGURA_SUB_ITER:-1})"
echo "  Constant obfusc.  : ${KAGURA_ENABLE_CO:-0}"
echo "  ObjC obfuscation  : ${KAGURA_ENABLE_OBJC:-1}"
echo "  Anti-debug/Frida  : ${KAGURA_ENABLE_ANTIDEB:-1}"
echo "════════════════════════════════════════"
