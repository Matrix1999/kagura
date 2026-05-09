#include <stdio.h>

/* Iterative fibonacci — avoids recursive-call interactions with FLA
   which rewrites the CFG dispatch but does not alter call semantics. */
static int fib(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        int t = a + b;
        a = b;
        b = t;
    }
    return b;
}

int main(void) {
    printf("%d\n", fib(10)); /* 55  */
    printf("%d\n", fib(11)); /* 89  */
    printf("%d\n", fib(12)); /* 144 */
    return 0;
}
