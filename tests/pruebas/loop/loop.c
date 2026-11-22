
#include <stdio.h>

int main(void) {
    long long total = 0;

    for (long long i = 0; i < 20000000; i++) {
        total += (i * 3 + 7) % 1009;
    }

    printf("%lld\n", total);
    return 0;
}
