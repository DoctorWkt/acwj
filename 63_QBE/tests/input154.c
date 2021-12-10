#include <stdio.h>

int main()
{
  int x= 5;
  long y= x;
  int z= (int)y;
  printf("%d %ld %d\n", x, y, z);
  return(0);
}
