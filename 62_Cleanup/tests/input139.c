#include <stdio.h>

int same(int x) { return(x); }

int main() {
  int a= 3;

  if (same(a) && same(a) >= same(a))
    printf("same apparently\n");
  return(0);
}
