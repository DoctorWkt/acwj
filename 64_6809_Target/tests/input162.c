#include <stdio.h>
#include <string.h>

void keyword(char *s) {

printf("search on %s: ", s); 
  switch (*s) {
    case 'b': printf("Starts with b\n"); break;
    case 'c': if (!strcmp(s, "case")) {
		printf("It's case\n"); return;
	      }
	      printf("Starts with c\n"); break;
    case 'v': if (!strcmp(s, "void")) {
                printf("It's void\n"); return;
              }
	      printf("Starts with v\n"); break;
    default: printf("Not found\n");
  }
}

int main() {
  keyword("char");
  keyword("case");
  keyword("break");
  keyword("horse");
  keyword("void");
  keyword("piano");
  return(0);
}
