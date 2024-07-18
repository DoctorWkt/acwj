#include <stdio.h>

enum {
  C_GLOBAL = 1,                 // Globally visible symbol
  C_LOCAL,                      // Locally visible symbol
  C_PARAM,                      // Locally visible function parameter
  C_EXTERN,                     // External globally visible symbol
  C_STATIC,                     // Static symbol, visible in one file
  C_STRUCT,                     // A struct
  C_UNION,                      // A union
  C_MEMBER,                     // Member of a struct or union
  C_ENUMTYPE,                   // A named enumeration type
  C_ENUMVAL,                    // A named enumeration value
  C_TYPEDEF,                    // A named typedef
  C_STRLIT                      // Not a class: used to denote string literals
};

void fred(int class) {
  char qbeprefix;
  int *silly;
  silly = &class;

  // Get the relevant QBE prefix for the symbol
  qbeprefix = ((class == C_GLOBAL) || (class == C_STATIC) ||
               (class == C_EXTERN)) ? (char)'$' : (char)'%';
  printf("class %d prefix %c\n", class, qbeprefix);

}

int main() {
  int i;

  for (i= C_GLOBAL; i<= C_STRLIT; i++)
    fred(i);
  return(0);
}
