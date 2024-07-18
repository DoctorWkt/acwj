#include "defs.h"
#include "data.h"
#include "tree.h"

// AST Tree Optimisation Code
// Copyright (c) 2019 Warren Toomey, GPL3

// Fold an AST tree with a unary operator
// and one INTLIT children. Return either 
// the original tree or a new leaf node.
static struct ASTnode *fold1(struct ASTnode *n) {
  int val;

  // Get the child value. Do the
  // operation if recognised.
  // Return the new leaf node.
  val = n->left->a_intvalue;
  switch (n->op) {
    case A_WIDEN:
      break;
    case A_INVERT:
      val = ~val;
      break;
    case A_LOGNOT:
      val = !val;
      break;
    case A_SCALE:
      val = val * n->a_intvalue;
      break;
    default:
      return (n);
  }

  // Return a leaf node with the new value
  return (mkastleaf(A_INTLIT, n->type, NULL, NULL, val));
}

// Fold an AST tree with a binary operator
// and two A_INTLIT children. Return either 
// the original tree or a new leaf node.
static struct ASTnode *fold2(struct ASTnode *n) {
  int val, leftval, rightval;

  // Get the values from each child
  leftval = n->left->a_intvalue;
  rightval = n->right->a_intvalue;

  // Perform some of the binary operations.
  // For any AST op we can't do, return
  // the original tree.
  switch (n->op) {
    case A_ADD:
      val = leftval + rightval;
      break;
    case A_SUBTRACT:
      val = leftval - rightval;
      break;
    case A_MULTIPLY:
      val = leftval * rightval;
      break;
    case A_DIVIDE:
      // Don't try to divide by zero.
      if (rightval == 0)
	return (n);
      val = leftval / rightval;
      break;
    case A_AND:
      val = leftval & rightval;
      break;
    case A_OR:
      val = leftval | rightval;
      break;
    case A_XOR:
      val = leftval ^ rightval;
      break;
    case A_LSHIFT:
      val = leftval << rightval;
      break;
    case A_RSHIFT:
      val = leftval >> rightval;
      break;
    default:
      return (n);
  }

  // Return a leaf node with the new value
  return (mkastleaf(A_INTLIT, n->type, NULL, NULL, val));
}

// Optimise an AST tree with
// a depth-first node traversal
struct ASTnode *optimise(struct ASTnode *n) {

  if (n == NULL) return (NULL);

  // Optimise the left child then the right
  n->left = optimise(n->left);
  if (n->left!=NULL) n->leftid= n->left->nodeid;
  n->right = optimise(n->right);
  if (n->right!=NULL) n->rightid= n->right->nodeid;

  // Fold literal constants:
  // If both children are A_INTLITs, do a fold2()
  if (n->left && n->left->op == A_INTLIT) {
    if (n->right && n->right->op == A_INTLIT)
      n = fold2(n);
    else
      // If only the left is A_INTLIT, do a fold1()
      n = fold1(n);
  }

  // Return the possibly modified tree
  return (n);
}
