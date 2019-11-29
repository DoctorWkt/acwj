#include <stdio.h>

struct foo {
  int x;
  int y;
} fred, mary;

struct foo james;

int main() {
  fred.x= 5;
  mary.y= 6;
  printf("%d %d\n", fred.x, mary.y);
  return(0);
}
