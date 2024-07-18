#include <stdio.h>

void donothing() { }

int main() {
  int x=0;
  printf("Doing nothing... "); donothing();
  printf("nothing done\n");

  while (++x < 100) ;
  printf("x is now %d\n", x);

  return(0);
}
