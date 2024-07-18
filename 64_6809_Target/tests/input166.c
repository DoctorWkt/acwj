#include <stdio.h>

char str[5];
char *ptr;

int scanch(int *slash) {
  int c;
  *slash=0;

  c= *ptr; ptr++;
  if (c == '\\') {
    *slash=1;
    c= *ptr; ptr++;
    switch (c) {
      case 'n':
        return ('\n');
    }
  }
  return(c);
}

int main() {
  int c;
  int slash;
  ptr= str;
  str[0]= 'H';
  str[1]= 'i';
  str[2]= '\\';
  str[3]= 'n';
  str[4]= 0;

  while (1) {
    c= scanch(&slash);
    if (c==0) break;
    printf("c %d slash %d\n", c, slash);
  }
  return(0);
}
