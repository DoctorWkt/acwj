#include <stdio.h>
int main() {
  int x=0;

  while (x<10) {
    switch(x) {
      case 2: x++; break;
      case 4: x=7; continue;
    }
    printf("%d\n", x);
    x++;
  }
  
  return(0);
}
