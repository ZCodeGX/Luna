
#include <stdio.h>

long long fibonacci(int n) {
    if (n < 2)
        return n;

    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main(void) {
    long long total = 0;

    for (long long i = 0; i < 1000000; i++) {
        total += (i * 3 + 7) % 1009;
    }

    printf("C - BUCLE\n");
    printf("%lld\n", total);

    printf("C - FIBONACCI\n");
    printf("%lld\n", fibonacci(28));

    return 0;
}
