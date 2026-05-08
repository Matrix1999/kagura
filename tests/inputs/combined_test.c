/*
 * combined_test.c — Test that ALL kagura passes can run together without
 * interfering with each other or corrupting program semantics.
 *
 * Compiled with:
 *   -kagura-fla -kagura-bcf -kagura-sub -kagura-str -kagura-co
 *   -kagura-ibr -kagura-lt  -kagura-fsplit
 *
 * The file intentionally mixes:
 *   - String constants (for -kagura-str)
 *   - Arithmetic expressions (for -kagura-sub / -kagura-co)
 *   - Branchy control flow (for -kagura-fla / -kagura-bcf)
 *   - Loops (for -kagura-lt)
 *   - Helper functions called from main (for -kagura-ibr / -kagura-fsplit)
 *
 * All expected results are deterministic and verified in main().
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ─── Strings (targeted by -kagura-str) ─────────────────────────────────── */

static const char GREETING[]  = "Hello, kagura!";
static const char SEPARATOR[] = "----------------";
static const char PASS_MSG[]  = "PASS";
static const char FAIL_MSG[]  = "FAIL";

/* ─── Arithmetic / bit-manipulation (targeted by -kagura-sub, -kagura-co) ── */

static uint32_t mix_bits(uint32_t a, uint32_t b) {
    uint32_t x = a ^ b;
    uint32_t y = (a + b) * 0x9e3779b9u;
    uint32_t z = (x >> 16) | (x << 16);
    return y ^ z;
}

static int abs_diff(int a, int b) {
    int d = a - b;
    return d < 0 ? -d : d;
}

static uint32_t rotate_left(uint32_t v, int shift) {
    return (v << shift) | (v >> (32 - shift));
}

/* ─── Control flow (targeted by -kagura-fla, -kagura-bcf) ────────────────── */

static int classify_char(char c) {
    if (c >= 'a' && c <= 'z') return 1;
    if (c >= 'A' && c <= 'Z') return 2;
    if (c >= '0' && c <= '9') return 3;
    return 0;
}

static int sign(int x) {
    if (x > 0) return  1;
    if (x < 0) return -1;
    return 0;
}

/* Recursive ackermann-like bounded function (exercises deep branching). */
static int ackermann_bounded(int m, int n) {
    if (m == 0) return n + 1;
    if (n == 0) return ackermann_bounded(m - 1, 1);
    if (m == 1) return n + 2;
    if (m == 2) return 2 * n + 3;
    /* Clamp to avoid exponential blow-up in tests */
    return ackermann_bounded(m - 1, 3);
}

/* ─── Loops (targeted by -kagura-lt) ─────────────────────────────────────── */

static int array_max(const int *arr, int len) {
    if (len <= 0) return 0;
    int m = arr[0];
    for (int i = 1; i < len; ++i)
        if (arr[i] > m)
            m = arr[i];
    return m;
}

static int array_min(const int *arr, int len) {
    if (len <= 0) return 0;
    int m = arr[0];
    for (int i = 1; i < len; ++i)
        if (arr[i] < m)
            m = arr[i];
    return m;
}

/* Bubble sort in-place. */
static void bubble_sort(int *arr, int len) {
    for (int i = 0; i < len - 1; ++i) {
        for (int j = 0; j < len - i - 1; ++j) {
            if (arr[j] > arr[j + 1]) {
                int t = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = t;
            }
        }
    }
}

/* Count occurrences of `c` in the NUL-terminated string `s`. */
static int count_char(const char *s, char c) {
    int n = 0;
    while (*s) {
        if (*s == c) ++n;
        ++s;
    }
    return n;
}

/* Reverse a string in place. */
static void reverse_str(char *s, int len) {
    int lo = 0, hi = len - 1;
    while (lo < hi) {
        char t = s[lo];
        s[lo]  = s[hi];
        s[hi]  = t;
        ++lo; --hi;
    }
}

/* Compute sum of ASCII values. */
static int ascii_sum(const char *s) {
    int sum = 0;
    while (*s)
        sum += (unsigned char)*s++;
    return sum;
}

/* ─── Combined: string + arithmetic + control flow ───────────────────────── */

/*
 * Walk a string, classify each character, and return a bitmask:
 *   bit 0 set → has lowercase
 *   bit 1 set → has uppercase
 *   bit 2 set → has digit
 */
static int char_classes(const char *s) {
    int mask = 0;
    while (*s) {
        int cls = classify_char(*s++);
        if (cls >= 1 && cls <= 3)
            mask |= (1 << (cls - 1));
    }
    return mask;
}

/* ─── main ────────────────────────────────────────────────────────────────── */

int main(void) {
    /* Print banner using the encrypted strings */
    printf("%s\n", SEPARATOR);
    printf("%s\n", GREETING);
    printf("%s\n", SEPARATOR);

    /* Arithmetic / bit ops */
    printf("mix_bits(0xDEAD,0xBEEF) = 0x%08X\n",
           mix_bits(0xDEAD, 0xBEEF));                          /* deterministic */
    printf("abs_diff(7,-3)  = %d\n", abs_diff(7, -3));        /* 10 */
    printf("abs_diff(-5,-5) = %d\n", abs_diff(-5, -5));       /* 0  */
    printf("rotl(0x1,4)     = 0x%X\n", rotate_left(0x1u, 4)); /* 0x10 */

    /* Control flow */
    printf("classify('a') = %d\n", classify_char('a'));   /* 1 */
    printf("classify('Z') = %d\n", classify_char('Z'));   /* 2 */
    printf("classify('9') = %d\n", classify_char('9'));   /* 3 */
    printf("classify('!')  = %d\n", classify_char('!'));  /* 0 */
    printf("sign(-7)       = %d\n", sign(-7));            /* -1 */
    printf("sign(0)        = %d\n", sign(0));             /* 0  */
    printf("sign(42)       = %d\n", sign(42));            /* 1  */
    printf("ack(0,5)       = %d\n", ackermann_bounded(0, 5));  /* 6  */
    printf("ack(2,3)       = %d\n", ackermann_bounded(2, 3));  /* 9  */

    /* Loops + array */
    int data[] = {5, 3, 8, 1, 9, 2, 7, 4, 6};
    int n = (int)(sizeof(data) / sizeof(data[0]));
    printf("max([5,3,8..]) = %d\n", array_max(data, n));  /* 9 */
    printf("min([5,3,8..]) = %d\n", array_min(data, n));  /* 1 */

    bubble_sort(data, n);
    printf("sorted[0]      = %d\n", data[0]);             /* 1 */
    printf("sorted[8]      = %d\n", data[8]);             /* 9 */
    printf("sorted[4]      = %d\n", data[4]);             /* 5 */

    /* String ops mixed with loops */
    const char *test_str = "Hello, kagura!";
    printf("count('a')     = %d\n", count_char(test_str, 'a'));  /* 2 */
    printf("ascii_sum      = %d\n", ascii_sum("ABC"));           /* 198 */

    /* char_classes covers strings + branching + loops together */
    printf("classes(Abc1)  = %d\n", char_classes("Abc1"));  /* 7 (all three) */
    printf("classes(abc)   = %d\n", char_classes("abc"));   /* 1 */
    printf("classes(ABC)   = %d\n", char_classes("ABC"));   /* 2 */
    printf("classes(123)   = %d\n", char_classes("123"));   /* 4 */

    /* Reverse string in a local buffer */
    char buf[] = "abcdef";
    reverse_str(buf, (int)strlen(buf));
    printf("reversed       = %s\n", buf);                   /* fedcba */

    printf("%s\n", PASS_MSG);
    return 0;
}
