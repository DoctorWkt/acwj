#include "defs.h"
#include "data.h"
#include "decl.h"

// AST tree functions
// Copyright (c) 2019 Warren Toomey, GPL3

// Build and return a generic AST node
struct ASTnode *mkastnode(int op, int type,
			  struct ASTnode *left,
			  struct ASTnode *mid,
			  struct ASTnode *right, int intvalue) {
  struct ASTnode *n;

  // Malloc a new ASTnode
  n = (struct ASTnode *) malloc(sizeof(struct ASTnode));
  if (n == NULL)
    fatal("Unable to malloc in mkastnode()");

  // Copy in the field values and return it
  n->op = op;
  n->type = type;
  n->left = left;
  n->mid = mid;
  n->right = right;
  n->v.intvalue = intvalue;
  return (n);
}


// Make an AST leaf node
struct ASTnode *mkastleaf(int op, int type, int intvalue) {
  return (mkastnode(op, type, NULL, NULL, NULL, intvalue));
}

// Make a unary AST node: only one child
struct ASTnode *mkastunary(int op, int type, struct ASTnode *left,
			    int intvalue) {
  return (mkastnode(op, type, left, NULL, NULL, intvalue));
}
