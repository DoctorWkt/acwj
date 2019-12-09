#include <stdio.h>

int a;

int main() {
  printf("%d\n", 24 % 9);
  printf("%d\n", 31 % 11);
  a= 24; a %= 9; printf("%d\n",a);
  a= 31; a %= 11; printf("%d\n",a);
  return(0);
}
