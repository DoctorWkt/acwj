#include "defs.h"
#include "data.h"
#include "decl.h"

// Types and type handling
// Copyright (c) 2019 Warren Toomey, GPL3

// Return true if a type is an int type
// of any size, false otherwise
int inttype(int type) {
  return (((type & 0xf) == 0) && (type >= P_CHAR && type <= P_LONG));
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
  if (type == P_STRUCT || type == P_UNION)
    return (ctype->size);
  return (genprimsize(type));
}

// Given an AST tree and a type which we want it to become,
// possibly modify the tree by widening or scaling so that
// it is compatible with this type. Return the original tree
// if no changes occurred, a modified tree, or NULL if the
// tree is not compatible with the given type.
// If this will be part of a binary operation, the AST op is not zero.
struct ASTnode *modify_type(struct ASTnode *tree, int rtype,
			    struct symtable *rctype, int op) {
  int ltype;
  int lsize, rsize;

  ltype = tree->type;

  // For A_LOGOR and A_LOGAND, both types have to be int or pointer types
  if (op == A_LOGOR || op == A_LOGAND) {
    if (!inttype(ltype) && !ptrtype(ltype))
      return (NULL);
    if (!inttype(ltype) && !ptrtype(rtype))
      return (NULL);
    return (tree);
  }
  // XXX No idea on these yet
  if (ltype == P_STRUCT || ltype == P_UNION)
    fatal("Don't know how to do this yet");
  if (rtype == P_STRUCT || rtype == P_UNION)
    fatal("Don't know how to do this yet");

  // Compare scalar int types
  if (inttype(ltype) && inttype(rtype)) {

    // Both types same, nothing to do
    if (ltype == rtype)
      return (tree);

    // Get the sizes for each type
    lsize = typesize(ltype, NULL);
    rsize = typesize(rtype, NULL);

    // The tree's type size is too big and we can't narrow
    if (lsize > rsize)
      return (NULL);

    // Widen to the right
    if (rsize > lsize)
      return (mkastunary(A_WIDEN, rtype, NULL, tree, NULL, 0));
  }
  // For pointers
  if (ptrtype(ltype) && ptrtype(rtype)) {
    // We can compare them
    if (op >= A_EQ && op <= A_GE)
      return (tree);

    // A comparison of the same type for a non-binary operation is OK,
    // or when the left tree is of  `void *` type.
    if (op == 0 && (ltype == rtype || ltype == pointer_to(P_VOID)))
      return (tree);
  }
  // We can scale only on add and subtract operations
  if (op == A_ADD || op == A_SUBTRACT || op == A_ASPLUS || op == A_ASMINUS) {

    // Left is int type, right is pointer type and the size
    // of the original type is >1: scale the left
    if (inttype(ltype) && ptrtype(rtype)) {
      rsize = genprimsize(value_at(rtype));
      if (rsize > 1)
	return (mkastunary(A_SCALE, rtype, rctype, tree, NULL, rsize));
      else
	// No need to scale, but we need to widen to pointer size
	return (mkastunary(A_WIDEN, rtype, NULL, tree, NULL, 0));
    }
  }
  // If we get here, the types are not compatible
  return (NULL);
}
