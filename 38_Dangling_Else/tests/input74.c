#include <stdio.h>

int main() {
  int x;
  int y;
  y= 0;

  for (x=0; x < 5; x++) {
    switch(x) {
      case 1:  { y= 5; break; }
      case 2:  { y= 7; break; }
      case 3:  { y= 9; }
      default: { y= 100; }
    }
    printf("%d\n", y);
  }
  return(0);
}
