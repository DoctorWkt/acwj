#include <stdio.h>

int ary[5];

void fred(int *ptr) {		// Receive a pointer
  printf("%d\n", ptr[3]);
}

int main() {
  ary[3]= 2008;
  printf("%d\n", ary[3]);
  fred(ary);			// Pass ary as a pointer
  return(0);
}
