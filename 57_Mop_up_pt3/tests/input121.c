#include <stdio.h>

int x;
int y= 3;

int main() {
  for (y= 0; y < 10; y++) {
    x= (y < 4) ? y + 2 :
       (y > 7) ? 1000 : y + 9;
    printf("%d\n", x);
  }
  return(0);
}
