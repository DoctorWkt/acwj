#include <stdio.h>

int main() {
  int x;

  x= 10;
  // Dangling else test
  if (x > 5)
    if (x > 15)
      printf("x > 15\n");
    else
      printf("15 >= x > 5\n");
  else
    printf("5 >= x\n");
  return(0);
}
