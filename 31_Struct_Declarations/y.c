#include <stdio.h>
#include <stdlib.h>

enum {
  P_NONE, P_VOID = 16, P_CHAR = 32, P_INT = 48, P_LONG = 64,
  P_STRUCT=80
};

int Line= 1;

void fatald(char *s, int d) {
  fprintf(stderr, "%s:%d on line %d\n", s, d, Line);
  exit(1);
}

// Given a scalar type, an existing memory offset
// (which hasn't been allocated to anything yet)
// and a direction (1 is up, -1 is down), calculate
// and return a suitably aligned memory offset
// for this scalar type. This could be the original
// offset, or it could be above/below the original
int calc_aligned_offset(int type, int offset, int direction) {
  int alignment;

  // We don't need to do this on x86-64, but let's
  // align chars on any offset and align ints/pointers
  // on a 4-byte alignment
  switch(type) {
    case P_CHAR: return (offset);
    case P_INT:
    case P_LONG: break;
    default:     fatald("Bad type in calc_aligned_offset:", type);
  }

  // Here we have an int or a long. Align it on a 4-byte offset
  // I put the generic code here so it can be reused elsewhere.
  alignment= 4;
  offset = (offset + direction * (alignment-1)) & ~(alignment-1);
  return (offset);
}

int main() {
  for (int i=0; i < 20; i++)
    printf("%d -> %d\n", i, calc_aligned_offset(P_INT, i, 1));
  for (int i=0; i > -20; i--)
    printf("%d -> %d\n", i, calc_aligned_offset(P_INT, i, -1));
  return(0);
}
