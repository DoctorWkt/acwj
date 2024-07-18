#include <stdio.h>
#include <string.h>

int keyword(char *s) {

  switch (*s) {
    case 'b': return(1);
    case 'c': if (!strcmp(s, "case")) return(2);
	      return(3);
    case 'v': if (!strcmp(s, "void")) return(4);
	      return(5);
    default: return(6);
  }
  return(0);
}

int main() {
  int i;

  i= keyword("break");    printf("break    %d\n", i);
  i= keyword("case");     printf("case     %d\n", i);
  i= keyword("char");     printf("char     %d\n", i);
  i= keyword("void");     printf("void     %d\n", i);
  i= keyword("volatile"); printf("volatile %d\n", i);
  i= keyword("horse");    printf("horse    %d\n", i);
  i= keyword("piano");    printf("piano    %d\n", i);
  return(0);
}
