#include <stdio.h>
#include <string.h>

char *a= "Hello";
char *b= "Goodbye";
char *c= "Fisherman";

int main() {
  if (strcmp(a, b)) { printf("%s and %s are different\n", a, b); }
  if (strcmp(b, c)) { printf("%s and %s are different\n", b, c); }
  if (strcmp(a, c)) { printf("%s and %s are different\n", a, c); }
  if (strcmp(a, a)) { printf("%s and %s are different\n", a, a); }
  if (strcmp(b, b)) { printf("%s and %s are different\n", b, b); }
  if (strcmp(c, c)) { printf("%s and %s are different\n", c, c); }
  if (!strcmp(c, c)) { printf("%s and %s are the same\n", c, c); }
  return(0);
}
