#include <stdio.h>

int main() {
  int x= 65535;
  char y;
  char *str;

  y= (char )x; printf("0x%x\n", y);
  str= (char *)0; printf("0x%lx\n", (long)str);
  return(0);
}
