#include <stdio.h>

char fred[5];
char *s;

int main() {
  s= fred;

  *s = 'F'; s++;
  *s++ = 'r';
  *s++ = 'e';
  *s++ = 'd';
  *s++ = 0;
  printf("%s\n", fred);
  return(0);
}
