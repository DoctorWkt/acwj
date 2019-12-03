#include <stdio.h>
void fred(void);

void fred(void) {
  printf("fred says hello\n");
}

int main(void) {
  fred(); return(0);
}
