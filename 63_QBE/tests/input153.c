#include <stdio.h>

enum {
  C_GLOBAL = 1, C_LOCAL, C_PARAM, C_EXTERN, C_STATIC, C_STRUCT,
  C_UNION, C_MEMBER, C_ENUMTYPE, C_ENUMVAL, C_TYPEDEF
};

int main() {
  int class;
  int b;
  char q;

  for (class = C_GLOBAL; class <= C_TYPEDEF; class ++) {
    q = ((class == C_GLOBAL) || (class == C_STATIC) ||
                 (class == C_EXTERN)) ? '$' : '%';
    printf("class %d prefix %c\n", class, q);
  }
  return(0);
}
