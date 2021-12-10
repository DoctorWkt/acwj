#include "defs.h"
#include "data.h"
#include "decl.h"

// Generic code generator
// Copyright (c) 2019 Warren Toomey, GPL3

// Generate and return a new label number
static int labelid = 1;
int genlabel(void) {
  return (labelid++);
}

static void update_line(struct ASTnode *n) {
  // Output the line into the assembly if we've
  // changed the line number in the AST node
  if (n->linenum != 0 && Line != n->linenum) {
    Line = n->linenum;
    cglinenum(Line);
  }
}

// Generate the code for an IF statement
// and an optional ELSE clause.
static int genIF(struct ASTnode *n, int looptoplabel, int loopendlabel) {
  int Lfalse, Lend = 0;
  int r, r2;

  // Generate two labels: one for the
  // false compound statement, and one
  // for the end of the overall IF statement.
  // When there is no ELSE clause, Lfalse _is_
  // the ending label!
  Lfalse = genlabel();
  if (n->right)
    Lend = genlabel();

  // Generate the condition code
  r = genAST(n->left, Lfalse, NOLABEL, NOLABEL, n->op);
  // Test to see if the condition is true. If not, jump to the false label
  r2 = cgloadint(1, P_INT);
  cgcompare_and_jump(A_EQ, r, r2, Lfalse, P_INT);

  // Generate the true compound statement
  genAST(n->mid, NOLABEL, looptoplabel, loopendlabel, n->op);

  // If there is an optional ELSE clause,
  // generate the jump to skip to the end
  if (n->right) {
    // QBE doesn't like two jump instructions in a row, and
    // a break at the end of a true IF section causes this. The
    // solution is to insert a label before the IF jump.
    cglabel(genlabel());
    cgjump(Lend);
  }
  // Now the false label
  cglabel(Lfalse);

  // Optional ELSE clause: generate the
  // false compound statement and the
  // end label
  if (n->right) {
    genAST(n->right, NOLABEL, NOLABEL, loopendlabel, n->op);
    cglabel(Lend);
  }

  return (NOREG);
}

// Generate the code for a WHILE statement
static int genWHILE(struct ASTnode *n) {
  int Lstart, Lend;
  int r, r2;

  // Generate the start and end labels
  // and output the start label
  Lstart = genlabel();
  Lend = genlabel();
  cglabel(Lstart);

  // Generate the condition code
  r = genAST(n->left, Lend, Lstart, Lend, n->op);
  // Test to see if the condition is true. If not, jump to the end label
  r2 = cgloadint(1, P_INT);
  cgcompare_and_jump(A_EQ, r, r2, Lend, P_INT);

  // Generate the compound statement for the body
  genAST(n->right, NOLABEL, Lstart, Lend, n->op);

  // Finally output the jump back to the condition,
  // and the end label
  cgjump(Lstart);
  cglabel(Lend);
  return (NOREG);
}

// Generate the code for a SWITCH statement
static int genSWITCH(struct ASTnode *n) {
  int *caselabel;
  int Lend;
  int Lcode = 0;
  int i, reg, r2, type;
  struct ASTnode *c;

  // Create an array for the case labels
  caselabel = (int *) malloc((n->a_intvalue + 1) * sizeof(int));
  if (caselabel == NULL)
    fatal("malloc failed in genSWITCH");

  // Because QBE doesn't yet support jump tables,
  // we simply evaluate the switch condition and
  // then do successive comparisons and jumps,
  // just like we were doing successive if/elses

  // Generate a label for the end of the switch statement.
  Lend = genlabel();

  // Generate labels for each case. Put the end label in
  // as the entry after all the cases
  for (i = 0, c = n->right; c != NULL; i++, c = c->right)
    caselabel[i] = genlabel();
  caselabel[i] = Lend;

  // Output the code to calculate the switch condition
  reg = genAST(n->left, NOLABEL, NOLABEL, NOLABEL, 0);
  type = n->left->type;

  // Walk the right-child linked list to
  // generate the code for each case
  for (i = 0, c = n->right; c != NULL; i++, c = c->right) {

    // Generate a label for the actual code that the cases will fall down to
    if (Lcode == 0)
      Lcode = genlabel();

    // Output the label for this case's test
    cglabel(caselabel[i]);

    // Do the comparison and jump, but not if it's the default case
    if (c->op != A_DEFAULT) {
      // Jump to the next case if the value doesn't match the case value
      r2 = cgloadint(c->a_intvalue, type);
      cgcompare_and_jump(A_EQ, reg, r2, caselabel[i + 1], type);

      // Otherwise, jump to the code to handle this case
      cgjump(Lcode);
    }
    // Generate the case code. Pass in the end label for the breaks.
    // If case has no body, we will fall into the following body.
    // Reset Lcode so we will create a new code label on the next loop.
    if (c->left) {
      cglabel(Lcode);
      genAST(c->left, NOLABEL, NOLABEL, Lend, 0);
      Lcode = 0;
    }
  }

  // Now output the end label.
  cglabel(Lend);
  return (NOREG);
}

// Generate the code for an
// A_LOGAND or A_LOGOR operation
static int gen_logandor(struct ASTnode *n) {
  // Generate two labels
  int Lfalse = genlabel();
  int Lend = genlabel();
  int reg;
  int type;

  // Generate the code for the left expression
  // followed by the jump to the false label
  reg = genAST(n->left, NOLABEL, NOLABEL, NOLABEL, 0);
  type = n->left->type;
  cgboolean(reg, n->op, Lfalse, type);

  // Generate the code for the right expression
  // followed by the jump to the false label
  reg = genAST(n->right, NOLABEL, NOLABEL, NOLABEL, 0);
  type = n->right->type;
  cgboolean(reg, n->op, Lfalse, type);

  // We didn't jump so set the right boolean value
  if (n->op == A_LOGAND) {
    cgloadboolean(reg, 1, type);
    cgjump(Lend);
    cglabel(Lfalse);
    cgloadboolean(reg, 0, type);
  } else {
    cgloadboolean(reg, 0, type);
    cgjump(Lend);
    cglabel(Lfalse);
    cgloadboolean(reg, 1, type);
  }
  cglabel(Lend);
  return (reg);
}

// Generate the code to calculate the arguments of a
// function call, then call the function with these
// arguments. Return the temoprary that holds
// the function's return value.
static int gen_funccall(struct ASTnode *n) {
  struct ASTnode *gluetree;
  int i = 0, numargs = 0;
  int *arglist = NULL;
  int *typelist = NULL;

  // Determine the actual number of arguments
  for (gluetree = n->left; gluetree != NULL; gluetree = gluetree->left) {
    numargs++;
  }

  // Allocate memory to hold the list of argument temporaries.
  // We need to walk the list of arguments to determine the size
  for (i = 0, gluetree = n->left; gluetree != NULL; gluetree = gluetree->left)
    i++;

  if (i != 0) {
    arglist = (int *) malloc(i * sizeof(int));
    if (arglist == NULL)
      fatal("malloc failed in gen_funccall");
    typelist = (int *) malloc(i * sizeof(int));
    if (typelist == NULL)
      fatal("malloc failed in gen_funccall");
  }
  // If there is a list of arguments, walk this list
  // from the last argument (right-hand child) to the first.
  // Also cache the type of each expression
  for (i = 0, gluetree = n->left; gluetree != NULL; gluetree = gluetree->left) {
    // Calculate the expression's value
    arglist[i] =
      genAST(gluetree->right, NOLABEL, NOLABEL, NOLABEL, gluetree->op);
    typelist[i++] = gluetree->right->type;
  }

  // Call the function and return its result
  return (cgcall(n->sym, numargs, arglist, typelist));
}

// Generate code for a ternary expression
static int gen_ternary(struct ASTnode *n) {
  int Lfalse, Lend;
  int reg, expreg;
  int r, r2;

  // Generate two labels: one for the
  // false expression, and one for the
  // end of the overall expression
  Lfalse = genlabel();
  Lend = genlabel();

  // Generate the condition code
  r = genAST(n->left, Lfalse, NOLABEL, NOLABEL, n->op);
  // Test to see if the condition is true. If not, jump to the false label
  r2 = cgloadint(1, P_INT);
  cgcompare_and_jump(A_EQ, r, r2, Lfalse, P_INT);

  // Get a temporary to hold the result of the two expressions
  reg = cgalloctemp();

  // Generate the true expression and the false label.
  // Move the expression result into the known temporary.
  expreg = genAST(n->mid, NOLABEL, NOLABEL, NOLABEL, n->op);
  cgmove(expreg, reg, n->mid->type);
  cgjump(Lend);
  cglabel(Lfalse);

  // Generate the false expression and the end label.
  // Move the expression result into the known temporary.
  expreg = genAST(n->right, NOLABEL, NOLABEL, NOLABEL, n->op);
  cgmove(expreg, reg, n->right->type);
  cglabel(Lend);
  return (reg);
}

// Given an AST, an optional label, and the AST op
// of the parent, generate assembly code recursively.
// Return the temporary id with the tree's final value.
int genAST(struct ASTnode *n, int iflabel, int looptoplabel,
	   int loopendlabel, int parentASTop) {
  int leftreg = NOREG, rightreg = NOREG;
  int lefttype = P_VOID, type = P_VOID;
  struct symtable *leftsym = NULL;

  // Empty tree, do nothing
  if (n == NULL)
    return (NOREG);

  // Update the line number in the output
  update_line(n);

  // We have some specific AST node handling at the top
  // so that we don't evaluate the child sub-trees immediately
  switch (n->op) {
    case A_IF:
      return (genIF(n, looptoplabel, loopendlabel));
    case A_WHILE:
      return (genWHILE(n));
    case A_SWITCH:
      return (genSWITCH(n));
    case A_FUNCCALL:
      return (gen_funccall(n));
    case A_TERNARY:
      return (gen_ternary(n));
    case A_LOGOR:
      return (gen_logandor(n));
    case A_LOGAND:
      return (gen_logandor(n));
    case A_GLUE:
      // Do each child statement, and free the
      // temporaries after each child
      if (n->left != NULL)
	genAST(n->left, iflabel, looptoplabel, loopendlabel, n->op);
      if (n->right != NULL)
	genAST(n->right, iflabel, looptoplabel, loopendlabel, n->op);
      return (NOREG);
    case A_FUNCTION:
      // Generate the function's preamble before the code
      // in the child sub-tree
      cgfuncpreamble(n->sym);
      genAST(n->left, NOLABEL, NOLABEL, NOLABEL, n->op);
      cgfuncpostamble(n->sym);
      return (NOREG);
  }

  // General AST node handling below

  // Get the left and right sub-tree values. Also get the type
  if (n->left) {
    lefttype = type = n->left->type;
    leftsym = n->left->sym;
    leftreg = genAST(n->left, NOLABEL, NOLABEL, NOLABEL, n->op);
  }
  if (n->right) {
    type = n->right->type;
    rightreg = genAST(n->right, NOLABEL, NOLABEL, NOLABEL, n->op);
  }

  switch (n->op) {
    case A_ADD:
      return (cgadd(leftreg, rightreg, type));
    case A_SUBTRACT:
      return (cgsub(leftreg, rightreg, type));
    case A_MULTIPLY:
      return (cgmul(leftreg, rightreg, type));
    case A_DIVIDE:
      return (cgdivmod(leftreg, rightreg, A_DIVIDE, type));
    case A_MOD:
      return (cgdivmod(leftreg, rightreg, A_MOD, type));
    case A_AND:
      return (cgand(leftreg, rightreg, type));
    case A_OR:
      return (cgor(leftreg, rightreg, type));
    case A_XOR:
      return (cgxor(leftreg, rightreg, type));
    case A_LSHIFT:
      return (cgshl(leftreg, rightreg, type));
    case A_RSHIFT:
      return (cgshr(leftreg, rightreg, type));
    case A_EQ:
    case A_NE:
    case A_LT:
    case A_GT:
    case A_LE:
    case A_GE:
      return (cgcompare_and_set(n->op, leftreg, rightreg, lefttype));
    case A_INTLIT:
      return (cgloadint(n->a_intvalue, n->type));
    case A_STRLIT:
      return (cgloadglobstr(n->a_intvalue));
    case A_IDENT:
      // Load our value if we are an rvalue
      // or we are being dereferenced
      if (n->rvalue || parentASTop == A_DEREF) {
	return (cgloadvar(n->sym, n->op));
      } else
	return (NOREG);
    case A_ASPLUS:
    case A_ASMINUS:
    case A_ASSTAR:
    case A_ASSLASH:
    case A_ASMOD:
    case A_ASSIGN:

      // For the '+=' and friends operators, generate suitable code
      // and get the temporary with the result. Then take the left child,
      // make it the right child so that we can fall into the assignment code.
      switch (n->op) {
	case A_ASPLUS:
	  leftreg = cgadd(leftreg, rightreg, type);
	  n->right = n->left;
	  break;
	case A_ASMINUS:
	  leftreg = cgsub(leftreg, rightreg, type);
	  n->right = n->left;
	  break;
	case A_ASSTAR:
	  leftreg = cgmul(leftreg, rightreg, type);
	  n->right = n->left;
	  break;
	case A_ASSLASH:
	  leftreg = cgdivmod(leftreg, rightreg, A_DIVIDE, type);
	  n->right = n->left;
	  break;
	case A_ASMOD:
	  leftreg = cgdivmod(leftreg, rightreg, A_MOD, type);
	  n->right = n->left;
	  break;
      }

      // Now into the assignment code
      // Are we assigning to an identifier or through a pointer?
      switch (n->right->op) {
	case A_IDENT:
	  if (n->right->sym->class == C_GLOBAL ||
	      n->right->sym->class == C_EXTERN ||
	      n->right->sym->class == C_STATIC)
	    return (cgstorglob(leftreg, n->right->sym));
	  else
	    return (cgstorlocal(leftreg, n->right->sym));
	case A_DEREF:
	  return (cgstorderef(leftreg, rightreg, n->right->type));
	default:
	  fatald("Can't A_ASSIGN in genAST(), op", n->op);
      }
    case A_WIDEN:
      // Widen the child's type to the parent's type
      return (cgwiden(leftreg, lefttype, n->type));
    case A_RETURN:
      cgreturn(leftreg, Functionid);
      return (NOREG);
    case A_ADDR:
      // If we have a symbol, get its address. Otherwise,
      // the left temporary already has the address because
      // it's a member access
      if (n->sym != NULL)
	return (cgaddress(n->sym));
      else
	return (leftreg);
    case A_DEREF:
      // If we are an rvalue, dereference to get the value we point at,
      // otherwise leave it for A_ASSIGN to store through the pointer
      if (n->rvalue)
	return (cgderef(leftreg, lefttype));
      else
	return (leftreg);
    case A_SCALE:
      // Small optimisation: use shift if the
      // scale value is a known power of two
      switch (n->a_size) {
	case 2:
	  return (cgshlconst(leftreg, 1, type));
	case 4:
	  return (cgshlconst(leftreg, 2, type));
	case 8:
	  return (cgshlconst(leftreg, 3, type));
	default:
	  // Load a temporary with the size and
	  // multiply the leftreg by this size
	  rightreg = cgloadint(n->a_size, P_INT);
	  return (cgmul(leftreg, rightreg, type));
      }
    case A_POSTINC:
    case A_POSTDEC:
      // Load and decrement the variable's value into a temporary
      // and post increment/decrement it
      return (cgloadvar(n->sym, n->op));
    case A_PREINC:
    case A_PREDEC:
      // Load and decrement the variable's value into a temporary
      // and pre increment/decrement it
      return (cgloadvar(leftsym, n->op));
    case A_NEGATE:
      return (cgnegate(leftreg, type));
    case A_INVERT:
      return (cginvert(leftreg, type));
    case A_LOGNOT:
      return (cglognot(leftreg, type));
    case A_TOBOOL:
      // If the parent AST node is an A_IF or A_WHILE, generate
      // a compare followed by a jump. Otherwise, set the temporary
      // to 0 or 1 based on it's zeroeness or non-zeroeness
      return (cgboolean(leftreg, parentASTop, iflabel, type));
    case A_BREAK:
      cgjump(loopendlabel);
      return (NOREG);
    case A_CONTINUE:
      cgjump(looptoplabel);
      return (NOREG);
    case A_CAST:
      return (cgcast(leftreg, lefttype, n->type));
    default:
      fatald("Unknown AST operator", n->op);
  }
  return (NOREG);		// Keep -Wall happy
}

void genpreamble(char *filename) {
  cgpreamble(filename);
}

void genpostamble() {
  cgpostamble();
}

void genglobsym(struct symtable *node) {
  cgglobsym(node);
}

// Generate a global string.
// If append is true, append to
// previous genglobstr() call.
int genglobstr(char *strvalue, int append) {
  int l = genlabel();
  cgglobstr(l, strvalue, append);
  return (l);
}
void genglobstrend(void) {
  cgglobstrend();
}
int genprimsize(int type) {
  return (cgprimsize(type));
}
int genalign(int type, int offset, int direction) {
  return (cgalign(type, offset, direction));
}
