#include <stdio.h>

static int counter=0;
static int fred(void) { return(counter++); }

int main(void) {
  int i;
  for (i=0; i < 5; i++)
    printf("%d\n", fred());
  return(0);
}
