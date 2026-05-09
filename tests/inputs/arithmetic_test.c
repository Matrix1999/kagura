#include <stdio.h>
#include <stdint.h>

uint32_t obfuscate_me(uint32_t a, uint32_t b) {
    uint32_t x = a + b;
    uint32_t y = a - b;
    uint32_t z = a & b;
    uint32_t w = a | b;
    uint32_t v = a ^ b;
    uint32_t u = a * b;
    return x ^ y ^ z ^ w ^ v ^ u;
}

int main(void) {
    printf("%u\n", obfuscate_me(0xDEAD, 0xBEEF));
    return 0;
}
