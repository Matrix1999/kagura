#include <stdio.h>
#include <string.h>

// These strings should be encrypted by StringEncryptionPass
static const char API_KEY[] = "sk-test-1234567890abcdef";
static const char BASE_URL[] = "https://api.example.com/v1";
static const char SECRET[]  = "my_super_secret_key_do_not_leak";

int main(void) {
    printf("key len: %zu\n", strlen(API_KEY));
    printf("url len: %zu\n", strlen(BASE_URL));
    printf("sec len: %zu\n", strlen(SECRET));
    return 0;
}
