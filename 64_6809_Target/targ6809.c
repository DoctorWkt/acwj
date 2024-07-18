#include "defs.h"
#include "misc.h"
#include "types.h"

// Target-specific functions which get used
// by the parser as well as the code generator.
// Copyright (c) 2024 Warren Toomey, GPL3

// Given a scalar type value, return the
// size of the type in bytes.
int cgprimsize(int type) {
  if (ptrtype(type))
    return (2);
  switch (type) {
  case P_VOID:
    return (0);
  case P_CHAR:
    return (1);
  case P_INT:
    return (2);
  case P_LONG:
    return (4);
  default:
    fatald("Bad type in cgprimsize:", type);
  }
  return (0);                   // Keep -Wall happy
}

int cgalign(int type, int offset, int direction) {
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
  return(P_INT);
}
