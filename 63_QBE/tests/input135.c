#include <stdio.h>

void fred() {
  int x= 5;
  printf("testing x\n");
  if (x > 4) return;
  printf("x below 5\n");
}

int main() {
  fred();
  return(0);
}
