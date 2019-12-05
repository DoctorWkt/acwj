#include <stdio.h>

int main() {
  int x;
  x = 0;
  while (x < 100) {
    if (x == 5) { x = x + 2; continue; }
    printf("%d\n", x);
    if (x == 14) { break; }
    x = x + 1;
  }
  printf("Done\n");
  return (0);
}
