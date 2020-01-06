#include "defs.h"
#include "data.h"
#include "decl.h"

// Types and type handling
// Copyright (c) 2019 Warren Toomey, GPL3

// Given two primitive types,
// return true if they are compatible,
// false otherwise. Also return either
// zero or an A_WIDEN operation if one
// has to be widened to match the other.
// If onlyright is true, only widen left to right.
int type_compatible(int *left, int *right, int onlyright) {

  // Voids not compatible with anything
  if ((*left == P_VOID) || (*right == P_VOID))
    return (0);

  // Same types, they are compatible
  if (*left == *right) {
    *left = *right = 0;
    return (1);
  }

  // Widen P_CHARs to P_INTs as required
  if ((*left == P_CHAR) && (*right == P_INT)) {
    *left = A_WIDEN;
    *right = 0;
    return (1);
  }
  if ((*left == P_INT) && (*right == P_CHAR)) {
    if (onlyright)
      return (0);
    *left = 0;
    *right = A_WIDEN;
    return (1);
  }
  // Anything remaining is compatible
  *left = *right = 0;
  return (1);
}
