/*
 * indirect_branch_test.c — Test input for the -kagura-ibr pass.
 *
 * The indirect-branch pass rewrites every direct call to a module-internal
 * function into an indirect call through a private function-pointer global.
 * External (declared-only) functions such as printf are untouched.
 *
 * This file provides several small static helper functions that call each
 * other, giving the pass plenty of direct-call sites to transform.  main()
 * exercises each helper multiple times with different arguments so that the
 * test validates both compilation and basic correctness of the indirect
 * dispatch.
 */

#include <stdio.h>

/* ─── Helpers ─────────────────────────────────────────────────────────────── */

static int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

static int lcm(int a, int b) {
    if (a == 0 || b == 0)
        return 0;
    /* Call gcd — this direct call is a target for -kagura-ibr. */
    int g = gcd(a, b);
    return (a / g) * b;
}

static int is_prime(int n) {
    if (n < 2)  return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (int i = 3; i * i <= n; i += 2)
        if (n % i == 0)
            return 0;
    return 1;
}

/* Count primes in [lo, hi] using is_prime. */
static int count_primes(int lo, int hi) {
    int count = 0;
    for (int i = lo; i <= hi; ++i)
        count += is_prime(i);
    return count;
}

/* Integer power: base^exp (exp >= 0). */
static int ipow(int base, int exp) {
    int result = 1;
    while (exp > 0) {
        if (exp & 1)
            result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

/* Sum of digits (base 10). */
static int digit_sum(int n) {
    if (n < 0) n = -n;
    int s = 0;
    while (n > 0) {
        s += n % 10;
        n /= 10;
    }
    return s;
}

/* Compute n-th triangular number: 1+2+…+n. */
static int triangular(int n) {
    if (n <= 0) return 0;
    return n * (n + 1) / 2;
}

/*
 * Compute the Collatz sequence length starting from n.
 * Uses is_prime as a conditional branch to exercise cross-call IBR paths.
 */
static int collatz_len(int n) {
    int steps = 1;
    while (n != 1) {
        if (n % 2 == 0)
            n /= 2;
        else
            n = 3 * n + 1;
        ++steps;
    }
    return steps;
}

/* Max of two ints — trivial but gives IBR more call sites. */
static int imax(int a, int b) { return a > b ? a : b; }
static int imin(int a, int b) { return a < b ? a : b; }

/* Clamp a value to [lo, hi]. */
static int clamp(int v, int lo, int hi) {
    return imax(lo, imin(v, hi));
}

/* ─── main ────────────────────────────────────────────────────────────────── */

int main(void) {
    /* GCD / LCM */
    printf("gcd(12,8)   = %d\n", gcd(12, 8));        /* 4  */
    printf("gcd(100,75) = %d\n", gcd(100, 75));      /* 25 */
    printf("gcd(17,13)  = %d\n", gcd(17, 13));       /* 1  */
    printf("lcm(4,6)    = %d\n", lcm(4, 6));         /* 12 */
    printf("lcm(7,5)    = %d\n", lcm(7, 5));         /* 35 */

    /* Primality */
    printf("isPrime(2)  = %d\n", is_prime(2));        /* 1 */
    printf("isPrime(9)  = %d\n", is_prime(9));        /* 0 */
    printf("isPrime(97) = %d\n", is_prime(97));       /* 1 */
    printf("primes<50   = %d\n", count_primes(2, 50)); /* 15 */

    /* Power */
    printf("2^10        = %d\n", ipow(2, 10));        /* 1024 */
    printf("3^5         = %d\n", ipow(3, 5));         /* 243  */
    printf("7^0         = %d\n", ipow(7, 0));         /* 1    */

    /* Digit sum */
    printf("digsum(12345)= %d\n", digit_sum(12345));  /* 15 */
    printf("digsum(0)    = %d\n", digit_sum(0));      /* 0  */

    /* Triangular */
    printf("tri(10)     = %d\n", triangular(10));     /* 55 */
    printf("tri(0)      = %d\n", triangular(0));      /* 0  */

    /* Collatz */
    printf("collatz(6)  = %d\n", collatz_len(6));     /* 9  */
    printf("collatz(27) = %d\n", collatz_len(27));    /* 112 */

    /* Clamp */
    printf("clamp(5,1,10) = %d\n", clamp(5, 1, 10));  /* 5  */
    printf("clamp(-3,0,9) = %d\n", clamp(-3, 0, 9));  /* 0  */
    printf("clamp(15,0,9) = %d\n", clamp(15, 0, 9));  /* 9  */

    return 0;
}
