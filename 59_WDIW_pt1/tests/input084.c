#include <stdio.h>

int main() {
  int x, y;
  x=2; y=3;
  printf("%d %d\n", x, y);

  char a, *b;
  a= 'f'; b= &a;
  printf("%c %c\n", a, *b);

  return(0);
}
