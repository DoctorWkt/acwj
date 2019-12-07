#include <stdio.h>

char *str= "qwertyuiop";

int list[]= {3, 5, 7, 9, 11, 13, 15};

int *lptr;

int main() {
  printf("%c\n", *str);
  str= str + 1; printf("%c\n", *str);
  str += 1; printf("%c\n", *str);
  str += 1; printf("%c\n", *str);
  str -= 1; printf("%c\n", *str);

  lptr= list;
  printf("%d\n", *lptr);
  lptr= lptr + 1; printf("%d\n", *lptr);
  lptr += 1; printf("%d\n", *lptr);
  lptr += 1; printf("%d\n", *lptr);
  lptr -= 1; printf("%d\n", *lptr);
  return(0);
}
