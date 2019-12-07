#include <stdio.h>

static char *fred(void) {
  return("Hello");
}

int main(void) {
  printf("%s\n", fred());
  return(0);
}
