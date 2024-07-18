#include <stdio.h>
#include <string.h>

char *str= "Hello there, this is a sentence. How about that?";

int main() {
  char *comma, *dot;
  int diff;
  comma= strchr(str, ',');
  dot= strchr(str, '.');
  diff= dot - comma;
  printf("dot comma difference is %d\n", diff);
  return(0);
}
