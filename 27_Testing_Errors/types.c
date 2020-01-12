#include "defs.h"
#include "data.h"
#include "decl.h"

// Types and type handling
// Copyright (c) 2019 Warren Toomey, GPL3

// Return true if a type is an int type
// of any size, false otherwise
int inttype(int type) {
  if (type == P_CHAR || type == P_INT || type == P_LONG)
    return (1);
  return (0);
}

// Return true if a type is of pointer type
int ptrtype(int type) {
  if (type == P_VOIDPTR || type == P_CHARPTR ||
      type == P_INTPTR || type == P_LONGPTR)
    return (1);
  return (0);
}

// Given a primitive type, return
// the type which is a pointer to it
int pointer_to(int type) {
  int newtype;
  switch (type) {
  case P_VOID:
    newtype = P_VOIDPTR;
    break;
  case P_CHAR:
    newtype = P_CHARPTR;
    break;
  case P_INT:
    newtype = P_INTPTR;
    break;
  case P_LONG:
    newtype = P_LONGPTR;
    break;
  default:
    fatald("Unrecognised in pointer_to: type", type);
  }
  return (newtype);
}

// Given a primitive pointer type, return
// the type which it points to
int value_at(int type) {
  int newtype;
  switch (type) {
  case P_VOIDPTR:
    newtype = P_VOID;
    break;
  case P_CHARPTR:
    newtype = P_CHAR;
    break;
  case P_INTPTR:
    newtype = P_INT;
    break;
  case P_LONGPTR:
    newtype = P_LONG;
    break;
  default:
    fatald("Unrecognised in value_at: type", type);
  }
  return (newtype);
}

// Given an AST tree and a type which we want it to become,
// possibly modify the tree by widening or scaling so that
// it is compatible with this type. Return the original tree
// if no changes occurred, a modified tree, or NULL if the
// tree is not compatible with the given type.
// If this will be part of a binary operation, the AST op is not zero.
struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op) {
  int ltype;
  int lsize, rsize;

  ltype = tree->type;

  // Compare scalar int types
  if (inttype(ltype) && inttype(rtype)) {

    // Both types same, nothing to do
    if (ltype == rtype)
      return (tree);

    // Get the sizes for each type
    lsize = genprimsize(ltype);
    rsize = genprimsize(rtype);

    // Tree's size is too big
    if (lsize > rsize)
      return (NULL);

    // Widen to the right
    if (rsize > lsize)
      return (mkastunary(A_WIDEN, rtype, tree, 0));
  }
  // For pointers on the left
  if (ptrtype(ltype)) {
    // OK is same type on right and not doing a binary op
    if (op == 0 && ltype == rtype)
      return (tree);
  }
  // We can scale only on A_ADD or A_SUBTRACT operation
  if (op == A_ADD || op == A_SUBTRACT) {

    // Left is int type, right is pointer type and the size
    // of the original type is >1: scale the left
    if (inttype(ltype) && ptrtype(rtype)) {
      rsize = genprimsize(value_at(rtype));
      if (rsize > 1)
	return (mkastunary(A_SCALE, rtype, tree, rsize));
      else
	return (tree);		// Size 1, no need to scale
    }
  }
  // If we get here, the types are not compatible
  return (NULL);
}
