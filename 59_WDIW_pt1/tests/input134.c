#include <stdio.h>

char y = 'a';
char *x;

int main() {
  x= &y;        if (x && y == 'a') printf("1st match\n");
  x= NULL;      if (x && y == 'a') printf("2nd match\n");
  x= &y; y='b'; if (x && y == 'a') printf("3rd match\n");
  return(0);
}
