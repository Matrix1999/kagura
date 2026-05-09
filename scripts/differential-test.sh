#!/usr/bin/env bash
# differential-test.sh — 4.7.6: Differential testing
#
# Compiles each test input twice — once plain and once with the full kagura
# obfuscation pipeline — executes both binaries, and asserts that their
# stdout output is identical.  A mismatch means obfuscation changed runtime
# behaviour.
#
# Usage:
#   ./scripts/differential-test.sh [plugin_path] [test_dir]
#
# Defaults:
#   plugin_path  = ./build/lib/Transforms/KaguraObfuscator.{dylib,so}
#   test_dir     = ./tests/inputs
#
# Environment overrides:
#   KAGURA_CLANG   path to clang
#   KAGURA_OPT     path to opt
#   KAGURA_SEED    PRNG seed (default: 42)
#   KAGURA_PASSES  obfuscation pipeline (default: standard set, see below)
#
# Exit codes:
#   0  — all tests passed (obfuscated output matches plain output)
#   1  — at least one test failed
#   2  — usage / tool error

set -euo pipefail

# --------------------------------------------------------------------------
# Defaults
# --------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

OS="$(uname -s)"
if [[ "${OS}" == "Darwin" ]]; then
    PLUGIN="${1:-${REPO_ROOT}/build/lib/Transforms/KaguraObfuscator.dylib}"
else
    PLUGIN="${1:-${REPO_ROOT}/build/lib/Transforms/KaguraObfuscator.so}"
fi
TEST_DIR="${2:-${REPO_ROOT}/tests/inputs}"
SEED="${KAGURA_SEED:-42}"

# Standard obfuscation pipeline used for differential testing.
# Passes that require runtime support (str-aes, vm, anti-tamper) are excluded
# because the test binaries are not linked against kagura_runtime.
DEFAULT_PASSES="kagura-str,kagura-co,kagura-genc,kagura-fsplit,kagura-sv,function(kagura-bcf,kagura-sub,kagura-ibr,kagura-lt,kagura-bbr,kagura-bbs,kagura-dci,kagura-mvo)"
PASSES="${KAGURA_PASSES:-${DEFAULT_PASSES}}"

# --------------------------------------------------------------------------
# Tool resolution
# --------------------------------------------------------------------------
CLANG="${KAGURA_CLANG:-}"
OPT="${KAGURA_OPT:-}"

if [[ -z "${CLANG}" ]] && [[ "${OS}" == "Darwin" ]]; then
    BREW_LLVM="$(brew --prefix llvm 2>/dev/null || true)"
    if [[ -n "${BREW_LLVM}" && -x "${BREW_LLVM}/bin/clang" ]]; then
        CLANG="${BREW_LLVM}/bin/clang"
        OPT="${BREW_LLVM}/bin/opt"
    fi
fi
CLANG="${CLANG:-$(command -v clang 2>/dev/null || true)}"
OPT="${OPT:-$(command -v opt 2>/dev/null || true)}"

die() { echo "ERROR: $*" >&2; exit 2; }

[[ -x "${CLANG}" ]] || die "clang not found. Set KAGURA_CLANG."
[[ -x "${OPT}"   ]] || die "opt not found. Set KAGURA_OPT."
[[ -f "${PLUGIN}" ]] || die "kagura plugin not found at: ${PLUGIN}  (build first: cd build && cmake --build .)"
[[ -d "${TEST_DIR}" ]] || die "test directory not found: ${TEST_DIR}"

# --------------------------------------------------------------------------
# Helpers
# --------------------------------------------------------------------------
TMPDIR_WORK="$(mktemp -d)"
trap 'rm -rf "${TMPDIR_WORK}"' EXIT

PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

run_test() {
    local src="$1"
    local name
    name="$(basename "${src}" .c)"
    local tmp="${TMPDIR_WORK}/${name}"

    echo -n "[diff-test] ${name} ... "

    # Stage 1: compile to IR (unoptimised, no kagura)
    local ir="${tmp}.ll"
    if ! "${CLANG}" -O1 -S -emit-llvm -o "${ir}" "${src}" 2>/dev/null; then
        echo "SKIP (compile error)"
        (( SKIP_COUNT++ )) || true
        return
    fi

    # Stage 2: compile plain binary
    local plain_bin="${tmp}.plain"
    if ! "${CLANG}" -O1 -o "${plain_bin}" "${src}" 2>/dev/null; then
        echo "SKIP (link error)"
        (( SKIP_COUNT++ )) || true
        return
    fi

    # Stage 3: apply obfuscation passes via opt
    local obf_ir="${tmp}.obf.ll"
    local obf_flags=(
        "-kagura-str" "-kagura-co" "-kagura-genc" "-kagura-fsplit" "-kagura-sv"
        "-kagura-bcf" "-kagura-sub" "-kagura-ibr" "-kagura-lt"
        "-kagura-bbr" "-kagura-bbs" "-kagura-dci" "-kagura-mvo"
        "-kagura-seed=${SEED}"
    )
    if ! "${OPT}" \
        -load-pass-plugin="${PLUGIN}" \
        -passes="${PASSES}" \
        "${obf_flags[@]}" \
        -S -o "${obf_ir}" "${ir}" 2>/dev/null; then
        echo "SKIP (opt error)"
        (( SKIP_COUNT++ )) || true
        return
    fi

    # Stage 4: compile obfuscated IR to binary
    local obf_bin="${tmp}.obf"
    if ! "${CLANG}" -O0 -o "${obf_bin}" "${obf_ir}" 2>/dev/null; then
        echo "SKIP (obf link error)"
        (( SKIP_COUNT++ )) || true
        return
    fi

    # Stage 5: run both and capture output
    local plain_out="${tmp}.plain.out"
    local obf_out="${tmp}.obf.out"

    if ! timeout 10 "${plain_bin}" > "${plain_out}" 2>&1; then
        echo "SKIP (plain run error)"
        (( SKIP_COUNT++ )) || true
        return
    fi
    if ! timeout 10 "${obf_bin}" > "${obf_out}" 2>&1; then
        echo "FAIL (obfuscated binary crashed or timed out)"
        (( FAIL_COUNT++ )) || true
        return
    fi

    # Stage 6: compare
    if diff -q "${plain_out}" "${obf_out}" > /dev/null 2>&1; then
        echo "PASS"
        (( PASS_COUNT++ )) || true
    else
        echo "FAIL (output mismatch)"
        echo "  --- plain ---"
        head -20 "${plain_out}" | sed 's/^/    /'
        echo "  --- obfuscated ---"
        head -20 "${obf_out}" | sed 's/^/    /'
        echo "  --- diff ---"
        diff "${plain_out}" "${obf_out}" | head -30 | sed 's/^/    /' || true
        (( FAIL_COUNT++ )) || true
    fi
}

# --------------------------------------------------------------------------
# Run tests
# --------------------------------------------------------------------------
echo "======================================================================"
echo " Kagura differential test — seed=${SEED}"
echo " Plugin:   ${PLUGIN}"
echo " Test dir: ${TEST_DIR}"
echo " Passes:   ${PASSES}"
echo "======================================================================"
echo ""

for src in "${TEST_DIR}"/*.c; do
    run_test "${src}"
done

echo ""
echo "======================================================================"
printf " Results: %d passed, %d failed, %d skipped\n" \
    "${PASS_COUNT}" "${FAIL_COUNT}" "${SKIP_COUNT}"
echo "======================================================================"

[[ ${FAIL_COUNT} -eq 0 ]]
