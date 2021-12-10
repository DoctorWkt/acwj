#include <stdio.h>
char *y[] = { "fish", "cow", NULL };
char *z= NULL;

int main() {
  int i;
  char *ptr;
  for (i=0; i < 3; i++) {
    ptr= y[i];
    if (ptr != (char *)0)
      printf("%s\n", y[i]);
    else
      printf("NULL\n");
  }
  return(0);
}
