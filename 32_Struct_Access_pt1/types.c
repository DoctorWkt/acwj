#include "defs.h"
#include "data.h"
#include "decl.h"

// Types and type handling
// Copyright (c) 2019 Warren Toomey, GPL3

// Return true if a type is an int type
// of any size, false otherwise
int inttype(int type) {
  return ((type & 0xf) == 0);
}

// Return true if a type is of pointer type
int ptrtype(int type) {
  return ((type & 0xf) != 0);
}

// Given a primitive type, return
// the type which is a pointer to it
int pointer_to(int type) {
  if ((type & 0xf) == 0xf)
    fatald("Unrecognised in pointer_to: type", type);
  return (type + 1);
}

// Given a primitive pointer type, return
// the type which it points to
int value_at(int type) {
  if ((type & 0xf) == 0x0)
    fatald("Unrecognised in value_at: type", type);
  return (type - 1);
}

// Given a type and a composite type pointer, return
// the size of this type in bytes
int typesize(int type, struct symtable *ctype) {
  if (type == P_STRUCT)
    return(ctype->size);
  return(genprimsize(type));
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

  // XXX No idea on these yet
  if (ltype == P_STRUCT)
    fatal("Don't know how to do this yet");
  if (rtype == P_STRUCT)
    fatal("Don't know how to do this yet");

  // Compare scalar int types
  if (inttype(ltype) && inttype(rtype)) {

    // Both types same, nothing to do
    if (ltype == rtype)
      return (tree);

    // Get the sizes for each type
    lsize = typesize(ltype, NULL); // XXX Fix soon
    rsize = typesize(rtype, NULL); // XXX Fix soon

    // Tree's size is too big
    if (lsize > rsize)
      return (NULL);

    // Widen to the right
    if (rsize > lsize)
      return (mkastunary(A_WIDEN, rtype, tree, NULL, 0));
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
	return (mkastunary(A_SCALE, rtype, tree, NULL, rsize));
      else
	return (tree);		// Size 1, no need to scale
    }
  }
  // If we get here, the types are not compatible
  return (NULL);
}
