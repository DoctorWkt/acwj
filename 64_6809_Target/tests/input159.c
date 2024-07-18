#include <stdio.h>

enum {
  P_NONE, P_VOID = 16, P_CHAR = 32, P_INT = 48, P_LONG = 64,
  P_STRUCT=80, P_UNION=96
};

int inttype(int type) {
  return (((type & 0xf) == 0) && (type >= P_CHAR && type <= P_LONG));
}

int main() {

  printf("%d\n", inttype(P_NONE));
  printf("%d\n", inttype(P_VOID));
  printf("%d\n", inttype(P_CHAR));
  printf("%d\n", inttype(P_INT));
  printf("%d\n", inttype(P_LONG));
  printf("%d\n", inttype(P_STRUCT));
  printf("%d\n", inttype(P_UNION));
  printf("%d\n", inttype(P_CHAR+1));
  printf("%d\n", inttype(P_INT+1));
  return(0);
}
