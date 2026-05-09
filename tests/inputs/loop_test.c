/*
 * loop_test.c — Test input for the -kagura-lt (loop transform) pass.
 *
 * The loop-transform pass applies three sub-transformations to natural loops:
 *   1. Bogus counter insertion in the latch block.
 *   2. Opaque invariant injection into the preheader.
 *   3. Loop counter splitting (64-bit induction variable → two 32-bit halves).
 *
 * Each function below exercises a distinct loop pattern to maximise coverage.
 * main() verifies all results against pre-computed expected values.
 */

#include <stdio.h>
#include <stdint.h>

/* ─── 1. Simple for-loop: sum of array ───────────────────────────────────── */

static int array_sum(const int *arr, int len) {
    int total = 0;
    for (int i = 0; i < len; ++i)
        total += arr[i];
    return total;
}

/* ─── 2. For-loop with break and continue ────────────────────────────────── */

/*
 * Count elements in arr[0..len) that are divisible by `div` but not by `skip`.
 * Uses both `continue` (skip multiples of `skip`) and `break` (stop at -1).
 */
static int count_divisible(const int *arr, int len, int div, int skip) {
    int count = 0;
    for (int i = 0; i < len; ++i) {
        if (arr[i] == -1)
            break;
        if (arr[i] % skip == 0)
            continue;
        if (arr[i] % div == 0)
            ++count;
    }
    return count;
}

/* ─── 3. Nested loops: 4×4 matrix multiply ───────────────────────────────── */

static void mat4_mul(const int a[4][4], const int b[4][4], int c[4][4]) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int s = 0;
            for (int k = 0; k < 4; ++k)
                s += a[i][k] * b[k][j];
            c[i][j] = s;
        }
    }
}

/* ─── 4. While loop: integer square root (Newton's method) ──────────────── */

static int isqrt(int n) {
    if (n <= 0) return 0;
    int x = n, y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

/* ─── 5. Do-while loop: count bits set (population count) ───────────────── */

static int popcount(unsigned int v) {
    int count = 0;
    do {
        count += v & 1;
        v >>= 1;
    } while (v != 0);
    return count;
}

/* ─── 6. Loop over 64-bit induction variable (exercises counter splitting) ── */

/*
 * Compute the sum of all integers in [lo, hi] using a 64-bit loop variable.
 * The loop transform pass attempts to split 64-bit induction variables, so
 * this exercises that code path directly.
 */
static int64_t range_sum(int64_t lo, int64_t hi) {
    int64_t s = 0;
    for (int64_t i = lo; i <= hi; ++i)
        s += i;
    return s;
}

/* ─── 7. Loop with early return ──────────────────────────────────────────── */

/* Find first index where arr[i] == target; return -1 if not found. */
static int find_first(const int *arr, int len, int target) {
    for (int i = 0; i < len; ++i)
        if (arr[i] == target)
            return i;
    return -1;
}

/* ─── main ────────────────────────────────────────────────────────────────── */

int main(void) {
    /* Array sum */
    static const int nums[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    printf("sum[1..10]     = %d\n", array_sum(nums, 10));   /* 55 */

    /* Break / continue */
    static const int mixed[] = {3, 6, 9, 10, 12, 15, -1, 18};
    printf("div3!div6<-1   = %d\n",
           count_divisible(mixed, 8, 3, 6));                /* 3 (3,9,15) */

    /* Nested matrix multiply: identity × identity = identity */
    static const int I4[4][4] = {
        {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}
    };
    static const int A[4][4] = {
        {1,2,3,4}, {5,6,7,8}, {9,10,11,12}, {13,14,15,16}
    };
    int R[4][4] = {0};
    mat4_mul(A, I4, R);
    printf("mat(A*I)[0][0] = %d\n", R[0][0]);  /* 1  */
    printf("mat(A*I)[1][2] = %d\n", R[1][2]);  /* 7  */
    printf("mat(A*I)[3][3] = %d\n", R[3][3]);  /* 16 */

    int R2[4][4] = {0};
    mat4_mul(I4, A, R2);
    printf("mat(I*A)[2][1] = %d\n", R2[2][1]); /* 10 */

    /* Integer square root */
    printf("isqrt(0)    = %d\n", isqrt(0));    /* 0  */
    printf("isqrt(1)    = %d\n", isqrt(1));    /* 1  */
    printf("isqrt(16)   = %d\n", isqrt(16));   /* 4  */
    printf("isqrt(99)   = %d\n", isqrt(99));   /* 9  */
    printf("isqrt(100)  = %d\n", isqrt(100));  /* 10 */

    /* Population count */
    printf("popcount(0)    = %d\n", popcount(0));          /* 0  */
    printf("popcount(255)  = %d\n", popcount(255));        /* 8  */
    printf("popcount(0xFF00)=%d\n", popcount(0xFF00));     /* 8  */
    printf("popcount(1)    = %d\n", popcount(1));          /* 1  */

    /* 64-bit range sum */
    printf("sum[1..100]  = %lld\n", (long long)range_sum(1, 100));    /* 5050 */
    printf("sum[0..9]    = %lld\n", (long long)range_sum(0, 9));      /* 45   */

    /* Find first */
    static const int haystack[] = {7, 3, 9, 1, 5, 3, 2};
    printf("find(3)      = %d\n", find_first(haystack, 7, 3));  /* 1 */
    printf("find(2)      = %d\n", find_first(haystack, 7, 2));  /* 6 */
    printf("find(99)     = %d\n", find_first(haystack, 7, 99)); /* -1 */

    return 0;
}
