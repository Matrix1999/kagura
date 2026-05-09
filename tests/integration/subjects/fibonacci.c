#include <stdio.h>

static int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(void) {
    printf("%d\n", fib(10)); /* 55  */
    printf("%d\n", fib(11)); /* 89  */
    printf("%d\n", fib(12)); /* 144 */
    return 0;
}
