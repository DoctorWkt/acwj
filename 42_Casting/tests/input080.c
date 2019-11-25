#include <stdio.h> 

int x;
int y;

int main() {

  for (x=0, y=1; x < 6; x++, y=y+2) {
    printf("%d %d\n", x, y);
  }

  return(0);
}
