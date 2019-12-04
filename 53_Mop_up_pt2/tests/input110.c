#include <stdio.h>

int x;
int y;

int main() {
  x= 3; y= 15; y += x; printf("%d\n", y);
  x= 3; y= 15; y -= x; printf("%d\n", y);
  x= 3; y= 15; y *= x; printf("%d\n", y);
  x= 3; y= 15; y /= x; printf("%d\n", y);
  return(0);
}
