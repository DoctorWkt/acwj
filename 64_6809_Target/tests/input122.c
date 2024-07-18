#include <stdio.h>

int x, y, z1, z2;

int main() {
  for (x= 0; x <= 1; x++) {
    for (y= 0; y <= 1; y++) {
      z1= x || y; z2= x && y;
      printf("x %d, y %d, x || y %d, x && y %d\n", x, y, z1, z2);
    }
  }
  
  //z= x || y;
  return(0);
}
