#include <stdio.h> 

int x;
int y;

int main() {

  x= 0; y=1;

  for (;x<5;) {
    printf("%d %d\n", x, y);
    x=x+1;
    y=y+2;
  }

  return(0);
}
