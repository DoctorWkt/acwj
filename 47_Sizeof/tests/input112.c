#include <stdio.h>
char* y = NULL;
int x= 10 + 6;
int fred [ 2 + 3 ];

int main() {
  fred[2]= x;
  printf("%d\n", fred[2]);
  return(0);
}
