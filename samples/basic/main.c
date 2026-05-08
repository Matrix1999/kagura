/*
 * kagura sample: basic/main.c
 *
 * Demonstrates compile-time protection with all major kagura passes.
 *
 * Build (manual):
 *   clang -O1 \
 *     -fpass-plugin=/path/to/KaguraObfuscator.dylib \
 *     -mllvm -kagura-fla  \
 *     -mllvm -kagura-bcf  \
 *     -mllvm -kagura-sub  \
 *     -mllvm -kagura-str  \
 *     -mllvm -kagura-co   \
 *     -mllvm -kagura-ibr  \
 *     -mllvm -kagura-lt   \
 *     -mllvm -kagura-genc \
 *     main.c -o sample
 *
 * Build (CMake — see CMakeLists.txt in this directory):
 *   cmake -B build -DKAGURA_DIR=/path/to/kagura .
 *   cmake --build build
 *
 * Per-function protection can be toggled with compiler macros:
 *   -DKAGURA_ENABLE_VM=1      -- virtualize annotated functions
 *   -DKAGURA_ENABLE_ANTIDEB=1 -- enable anti-debug checks
 *
 * Source annotations supported by kagura:
 *   __attribute__((annotate("kagura.vm")))    -- force VM obfuscation
 *   __attribute__((annotate("kagura.fla")))   -- force CFG flattening
 *   __attribute__((annotate("kagura.bcf")))   -- force bogus CF injection
 *   __attribute__((annotate("kagura.skip")))  -- exclude from all passes
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ──────────────────────────────────────────────────────────────────────────
 * Annotation helpers
 *
 * When kagura is not in the build, these expand to nothing so the same
 * source compiles cleanly with any clang toolchain.
 * ────────────────────────────────────────────────────────────────────────── */

#ifdef __clang__
#  define KAGURA_VM    __attribute__((annotate("kagura.vm")))
#  define KAGURA_FLA   __attribute__((annotate("kagura.fla")))
#  define KAGURA_BCF   __attribute__((annotate("kagura.bcf")))
#  define KAGURA_SKIP  __attribute__((annotate("kagura.skip")))
#else
#  define KAGURA_VM
#  define KAGURA_FLA
#  define KAGURA_BCF
#  define KAGURA_SKIP
#endif

/* ──────────────────────────────────────────────────────────────────────────
 * License key validation
 *
 * Annotated with "kagura.vm": the function body will be virtualized into
 * kagura's custom stack-based bytecode.  This is the highest protection
 * level and is appropriate for small, high-value functions.
 *
 * The algorithm is intentionally simple (XOR + checksum) so the sample
 * compiles and runs correctly without external dependencies.
 * ────────────────────────────────────────────────────────────────────────── */

#define LICENSE_KEY_LEN  16
#define LICENSE_MAGIC    0xDEADBEEFu

KAGURA_VM
static int validate_license(const char *key, size_t len)
{
    if (!key || len != LICENSE_KEY_LEN)
        return 0;

    /* Constant is obfuscated by -kagura-co at compile time */
    uint32_t acc = LICENSE_MAGIC;
    for (size_t i = 0; i < len; ++i) {
        acc ^= (uint8_t)key[i];
        acc  = (acc << 5) | (acc >> 27);   /* rotate left 5 */
        acc += (uint32_t)i * 0x9e3779b9u;  /* Knuth multiplicative hash step */
    }

    /* The expected checksum for the hard-coded demo key below */
    return (acc == 0x4F3A1C2Bu);
}

/* ──────────────────────────────────────────────────────────────────────────
 * String decryption stub
 *
 * With -kagura-str the static xor_key and the string literal "Hello, kagura!"
 * are XOR-encrypted in the binary.  This function is annotated with
 * "kagura.fla" to additionally flatten its control flow.
 *
 * In production use you would call this with your own key/ciphertext pairs;
 * here it just demonstrates the pattern so IR analysis of the binary cannot
 * trivially read the plaintext string.
 * ────────────────────────────────────────────────────────────────────────── */

#define DECRYPT_BUFSIZE 64

KAGURA_FLA
static void decrypt_string(const uint8_t *cipher, size_t len,
                            uint8_t key, char *out)
{
    for (size_t i = 0; i < len && i < DECRYPT_BUFSIZE - 1; ++i)
        out[i] = (char)(cipher[i] ^ (key + (uint8_t)i));
    out[len < DECRYPT_BUFSIZE ? len : DECRYPT_BUFSIZE - 1] = '\0';
}

/* ──────────────────────────────────────────────────────────────────────────
 * Fibonacci calculator
 *
 * Annotated with "kagura.bcf": bogus dead blocks are injected around the
 * recursive calls, making static analysis significantly harder.
 *
 * "kagura.skip" would suppress all passes on this function — shown as a
 * comment here for documentation purposes.
 * ────────────────────────────────────────────────────────────────────────── */

KAGURA_BCF
static uint64_t fibonacci(unsigned n)
{
    /* Iterative to avoid stack overflow for large n */
    if (n <= 1)
        return (uint64_t)n;

    uint64_t prev = 0, curr = 1;
    for (unsigned i = 2; i <= n; ++i) {
        uint64_t next = prev + curr;
        prev = curr;
        curr = next;
    }
    return curr;
}

/* ──────────────────────────────────────────────────────────────────────────
 * Insertion sort
 *
 * Receives no special annotation — it will still be processed by any passes
 * enabled globally via -mllvm flags (-kagura-sub, -kagura-co, etc.).
 *
 * Marking performance-critical sort routines with KAGURA_SKIP is an
 * option to avoid compile-time overhead or runtime slowdown:
 *
 *   KAGURA_SKIP static void isort(...) { ... }
 * ────────────────────────────────────────────────────────────────────────── */

static void isort(int *arr, size_t n)
{
    for (size_t i = 1; i < n; ++i) {
        int key = arr[i];
        size_t j = i;
        while (j > 0 && arr[j - 1] > key) {
            arr[j] = arr[j - 1];
            --j;
        }
        arr[j] = key;
    }
}

/* ──────────────────────────────────────────────────────────────────────────
 * Simple expression parser — state machine
 *
 * Demonstrates CFG flattening on a non-trivial switch-heavy function.
 * -kagura-fla will wrap the existing switch in an outer dispatch loop,
 * making it a switch-within-a-switch, which defeats pattern matching by
 * decompilers like Hex-Rays and Ghidra.
 *
 * The parser accepts the grammar:
 *   expr  := number ('+' number)*
 *   number := [0-9]+
 * ────────────────────────────────────────────────────────────────────────── */

typedef enum {
    PS_START   = 0,
    PS_NUMBER  = 1,
    PS_EXPECT_OP = 2,
    PS_OPERATOR  = 3,
    PS_DONE    = 4,
    PS_ERROR   = 5
} parse_state_t;

KAGURA_FLA
static long parse_sum(const char *input)
{
    parse_state_t state = PS_START;
    long result = 0;
    long current = 0;

    for (const char *p = input; ; ++p) {
        char c = *p;

        switch (state) {
        case PS_START:
            if (c >= '0' && c <= '9') {
                current = c - '0';
                state = PS_NUMBER;
            } else if (c == '\0') {
                state = PS_ERROR;
            }
            break;

        case PS_NUMBER:
            if (c >= '0' && c <= '9') {
                current = current * 10 + (c - '0');
            } else {
                result += current;
                current = 0;
                state = PS_EXPECT_OP;
                --p; /* re-process this character */
            }
            break;

        case PS_EXPECT_OP:
            if (c == '+') {
                state = PS_OPERATOR;
            } else if (c == '\0') {
                state = PS_DONE;
            } else {
                state = PS_ERROR;
            }
            break;

        case PS_OPERATOR:
            if (c >= '0' && c <= '9') {
                current = c - '0';
                state = PS_NUMBER;
            } else {
                state = PS_ERROR;
            }
            break;

        case PS_DONE:
        case PS_ERROR:
            goto done;
        }

        if (c == '\0')
            break;
    }

done:
    return (state == PS_DONE || state == PS_EXPECT_OP) ? result : -1;
}

/* ──────────────────────────────────────────────────────────────────────────
 * Anti-debug guard (compile-time optional)
 *
 * Only compiled when KAGURA_ENABLE_ANTIDEB is defined.  At link time,
 * kagura_anti_debug_init() is provided by libkagura_runtime.a.
 * ────────────────────────────────────────────────────────────────────────── */

#if defined(KAGURA_ENABLE_ANTIDEB) && KAGURA_ENABLE_ANTIDEB
extern void kagura_anti_debug_init(void);
#else
static inline void kagura_anti_debug_init(void) { /* no-op */ }
#endif

/* ──────────────────────────────────────────────────────────────────────────
 * main
 * ────────────────────────────────────────────────────────────────────────── */

int main(void)
{
    /* Anti-debug / anti-Frida check (no-op unless KAGURA_ENABLE_ANTIDEB=1) */
    kagura_anti_debug_init();

    /* ── License validation ──────────────────────────────────────────────── */
    /* The string literal below is XOR-encrypted by -kagura-str at compile
     * time.  It is decrypted by an auto-generated constructor before main()
     * is reached, so it appears as a normal string at runtime but is never
     * stored in plaintext in the binary's __TEXT,__cstring section. */
    const char *demo_key = "KAGURA-DEMO-1234";
    if (validate_license(demo_key, LICENSE_KEY_LEN)) {
        puts("License OK");
    } else {
        puts("License INVALID");
        return 1;
    }

    /* ── String decryption ───────────────────────────────────────────────── */
    /* Simulate a payload string that is stored as ciphertext */
    static const uint8_t cipher[] = {
        0x2b, 0x2c, 0x3a, 0x3b, 0x36, 0x0e, 0x52, 0x2e,
        0x31, 0x37, 0x3a, 0x35, 0x24, 0x21
    };
    char plain[DECRYPT_BUFSIZE];
    decrypt_string(cipher, sizeof(cipher), 0x47 /* 'G' */, plain);
    printf("Decrypted: %s\n", plain);

    /* ── Fibonacci ───────────────────────────────────────────────────────── */
    printf("fib(20) = %llu\n", (unsigned long long)fibonacci(20));
    printf("fib(40) = %llu\n", (unsigned long long)fibonacci(40));

    /* ── Sort ────────────────────────────────────────────────────────────── */
    int arr[] = {42, 7, 19, 3, 99, 1, 55, 28};
    size_t arr_len = sizeof(arr) / sizeof(arr[0]);
    isort(arr, arr_len);
    printf("Sorted:  ");
    for (size_t i = 0; i < arr_len; ++i)
        printf("%d%s", arr[i], i + 1 < arr_len ? ", " : "\n");

    /* ── Parser ──────────────────────────────────────────────────────────── */
    struct { const char *input; long expected; } cases[] = {
        { "1+2+3",        6  },
        { "100+200",      300 },
        { "0",            0  },
        { "bad input",   -1  },
    };
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        long got = parse_sum(cases[i].input);
        printf("parse_sum(\"%s\") = %ld  [%s]\n",
               cases[i].input, got,
               got == cases[i].expected ? "PASS" : "FAIL");
    }

    return 0;
}
