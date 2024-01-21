#include "stdio.h"

// recursion test 1

// recursively computes n!, assuming n is nonnegative
int factorial(int n){
    if (n == 0 || n == 1)
        return(1);
    return(n * factorial(n - 1));
}

int main(void) {
    printf("0! = %d\n", factorial(0));
    printf("1! = %d\n", factorial(1));
    printf("3! = %d\n", factorial(3));
    printf("4! = %d\n", factorial(4));
    printf("7! = %d\n", factorial(7));
    return(0);
}