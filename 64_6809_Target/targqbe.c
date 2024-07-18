#include "defs.h"
#include "misc.h"
#include "types.h"

// Target-specific functions which get used
// by the parser as well as the code generator.
// Copyright (c) 2024 Warren Toomey, GPL3

// Given a scalar type value, return the
// size of the QBE type in bytes.
int cgprimsize(int type) {
  if (ptrtype(type))
    return (8);
  switch (type) {
    case P_CHAR:
      return (1);
    case P_INT:
      return (4);
    case P_LONG:
      return (8);
    default:
      fatald("Bad type in cgprimsize:", type);
  }
  return (0);                   // Keep -Wall happy
}

// Given a scalar type, an existing memory offset
// (which hasn't been allocated to anything yet)
// and a direction (1 is up, -1 is down), calculate
// and return a suitably aligned memory offset
// for this scalar type. This could be the original
// offset, or it could be above/below the original
int cgalign(int type, int offset, int direction) {
  int alignment;

  // We don't need to do this on x86-64, but let's
  // align chars on any offset and align ints/pointers
  // on a 4-byte alignment
  switch (type) {
    case P_CHAR:
      break;
    default:
      // Align whatever we have now on a 4-byte alignment.
      // I put the generic code here so it can be reused elsewhere.
      alignment = 4;
      offset = (offset + direction * (alignment - 1)) & ~(alignment - 1);
  }
  return (offset);
}

int genprimsize(int type) {
  return (cgprimsize(type));
}

int genalign(int type, int offset, int direction) {
  return (cgalign(type, offset, direction));
}


// Return the primitive type that can hold an address.
// This is used when we need to add a INTLIT to a
// pointer.
int cgaddrint(void) {
  return(P_LONG);
}
