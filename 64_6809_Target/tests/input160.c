#include <stdio.h>

int x= '#';

int main() {
  switch(x) {
    case 'x': printf("An x\n"); break;
    case '#': printf("An #\n"); break;
    default:  printf("No idea\n");
  }
  return(0);
}
