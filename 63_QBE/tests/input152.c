#include <stdio.h>

void fred(int x) {
  int a = 2;
  int *b = &x;
  printf("%d %d %d\n", x, a, *b);
}

int main() {
  fred(4);
  return(0);
}
