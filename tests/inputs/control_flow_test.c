#include <stdio.h>

int classify(int x) {
    if (x < 0)       return -1;
    else if (x == 0) return 0;
    else if (x < 10) return 1;
    else if (x < 100) return 2;
    else              return 3;
}

int fibonacci(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int c = a + b;
        a = b;
        b = c;
    }
    return b;
}

int main(void) {
    printf("%d\n", classify(42));
    printf("%d\n", fibonacci(10));
    return 0;
}
