#include <stdio.h>

int a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8;

int main() {
  int x;
  x= ((((((a + b) + c) + d) + e) + f) + g) + h;
  x= a + (b + (c + (d + (e + (f + (g + h))))));
  printf("x is %d\n", x);
  return(0);
}
