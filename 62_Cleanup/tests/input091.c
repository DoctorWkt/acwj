#include <stdio.h>

int fred[] = { 1, 2, 3, 4, 5 };
int jim[10] = { 1, 2, 3, 4, 5 };

int main() {
  int i;
  for (i=0; i < 5; i++) printf("%d\n", fred[i]);
  for (i=0; i < 10; i++) printf("%d\n", jim[i]);
  return(0);
}
