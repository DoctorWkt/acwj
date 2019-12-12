#include <stdio.h>

int main() {
  int  i;
  int  ary[5];
  char z;

  // Write below the array
  z= 'H';

  // Fill the array
  for (i=0; i < 5; i++)
    ary[i]= i * i;

  // Write above the array
  i=14;

  // Print out the array
  for (i=0; i < 5; i++)
    printf("%d\n", ary[i]);

  // See if either side is OK
  printf("%d %c\n", i, z);
  return(0);
}
