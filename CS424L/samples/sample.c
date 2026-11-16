#include <stdio.h>

/* A small C program used to test the Lab 1 and Lab 2 programs.
   This block comment spans two lines and has the number 999 in it. */

int max(int a, int b) {
    if (a < b) // compare the two values
        return b;
    return a;
}

int main() {
    int count = 123;
    float price = 123.10;
    int debt = -1234;
    double rate = 2.5e3;
    int diff = count-5;      // 5 here is a constant, the '-' is subtraction
    char msg[] = "line 42";  /* numbers in strings are ignored */

    for (int i = 0; i <= 10; i++) {
        count += i;
    }
    while (count != 0 && debt < 0) {
        count--;
    }
    printf("%d %f\n", max(count, diff), price * rate);
    return 0;
}
