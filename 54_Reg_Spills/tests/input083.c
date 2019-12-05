#include <stdio.h>

int main() {
  int x;

  // Dangling else test.
  // We should not print anything for x<= 5
  for (x=0; x < 12; x++)
    if (x > 5)
      if (x > 10)
        printf("10 < %2d\n", x);
      else
        printf(" 5 < %2d <= 10\n", x);
  return(0);
}

