#include <stdio.h>

static int localOffset=0;

static int newlocaloffset(int size) {
  localOffset += (size > 4) ? size : 4;
  return (-localOffset);
}

int main() {
  int i, r;
  for (i=1; i <= 12; i++) {
    r= newlocaloffset(i);
    printf("%d %d\n", i, r);
  }
  return(0);
}
