#include <stdio.h>

int main(void) {
    double x = 1.0;
    int i = 0;

    while (i < 50000000) {
        x = x + 0.000001;
        x = x * 1.0000001;
        x = x - 0.000001;
        i++;
    }

    printf("%.15f\n", x);

    return 0;
}