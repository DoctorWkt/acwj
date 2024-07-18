#include <stdio.h>

int i;
int j;

int main() {

  for (i=1; i<=10; i++) {
    j= ((i==4)) ? 17 : 23;
    printf("%d\n", j);
  }
  return(0);
}
