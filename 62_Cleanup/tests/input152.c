#include "stdio.h"

// recursion test 2

/* recursively computes the nth fibonacci number
assuming n is nonnegatve */
int fib(int n){
    if (n == 0 || n == 1)
        return(n);
    return(fib(n - 1) + fib(n - 2));
}

// prints the first 11 numbers in the fibonacci sequence
int main(void){
    printf("%d %d %d %d %d %d %d %d %d %d %d",
    fib(0),
    fib(1),
    fib(2),
    fib(3),
    fib(4),
    fib(5),
    fib(6),
    fib(7),
    fib(8),
    fib(9),
    fib(10));
    return(0);
}