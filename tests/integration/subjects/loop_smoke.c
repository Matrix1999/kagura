#include <stdio.h>

int main(void) {
    /* sum 0..9 */
    int sum = 0;
    for (int i = 0; i < 10; i++) sum += i;
    printf("%d\n", sum); /* 45 */

    /* sum 1..13 = 91, but use sum 1..100 counting by skipping = 0+10+20+...+100 */
    int sum2 = 0;
    for (int i = 0; i <= 100; i += 10) sum2 += i; /* 0+10+20+...+100 = 550 */
    printf("%d\n", sum2); /* 550 */
    return 0;
}
