#include <stdio.h>

char foo;
char *a, *b, *c;

int main() {

  a= b= c= NULL;
  if (a==NULL || b==NULL || c==NULL)
    printf("One of the three is NULL\n");
  a= &foo;
  if (a==NULL || b==NULL || c==NULL)
    printf("One of the three is NULL\n");
  b= &foo;
  if (a==NULL || b==NULL || c==NULL)
    printf("One of the three is NULL\n");
  c= &foo;
  if (a==NULL || b==NULL || c==NULL)
    printf("One of the three is NULL\n");
  else
    printf("All  three  are non-NULL\n");

  return(0);
}
