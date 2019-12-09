#include <stdio.h>

int main() {
  int x;
  for (x=0; x < 20; x++)
    switch(x) {
      case 2:
      case 3:
      case 5:
      case 7:
      case 11: printf("%2d infant prime\n", x); break;
      case 13:
      case 17:
      case 19: printf("%2d teen   prime\n", x); break;
      case 0:
      case 1:
      case 4:
      case 6:
      case 8:
      case 9:
      case 10:
      case 12: printf("%2d infant composite\n", x); break;
      default: printf("%2d teen   composite\n", x); break;
    }

  return(0);
}
