#include <stdio.h>

struct Location {
  int type;             // One of the L_ values
  char *name;           // A symbol's name
  long intval;          // Offset, const value, label-id etc.
  int primtype;         // 6809 primiive type, see P_POINTER below
};

#define NUMFREEREGS 16
#define L_FREE 1
struct Location Locn[NUMFREEREGS];

int main() {
  int l=5;
  Locn[l].type = 23;
  printf("%d\n", Locn[l].type);
  return(0);
}
