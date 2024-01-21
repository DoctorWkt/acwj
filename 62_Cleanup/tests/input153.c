#include "stdio.h"

// mutual recursion test

// both of these assume the argument is nonnegative
int is_even(int n);
int is_odd(int n);

int is_even(int n){
    if (n == 0)
        return(1);
    return(is_odd(n - 1));
}

int is_odd(int n){
    if (n == 0)
        return(0);
    return(is_even(n - 1));
}

int main(void){
    printf("%d %d %d %d", is_even(14), is_even(59), is_odd(2), is_odd(37));
    return(1);
}