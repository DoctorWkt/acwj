#include "defs.h"
#include "data.h"
#include "cg.h"
#include "decl.h"
#include "gen.h"
#include "misc.h"
#include "target.h"
#include "tree.h"
#include "types.h"

// Generic code generator
// Copyright (c) 2019 Warren Toomey, GPL3

int genAST(struct ASTnode *n, int iflabel, int looptoplabel, int loopendlabel,
	   int parentASTop);

// Generate and return a new label number
static int labelid = 1;
int genlabel(void) {
  return (labelid++);
}

void genfreeregs(int keepreg) {
  cgfreeallregs(keepreg);
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
static int genIF(struct ASTnode *n, struct ASTnode *nleft,
		 struct ASTnode *nmid, struct ASTnode *nright,
		 int looptoplabel, int loopendlabel) {
  int Lfalse, Lend;

  // Generate two labels: one for the
  // false compound statement, and one
  // for the end of the overall IF statement.
  // When there is no ELSE clause, Lfalse _is_
  // the ending label!
  Lfalse = genlabel();
  if (nright)
    Lend = genlabel();

  // Generate the condition code followed
  // by a jump to the false label.
  genAST(nleft, Lfalse, looptoplabel, loopendlabel, n->op);
  genfreeregs(NOREG);

  // Generate the true compound statement
  genAST(nmid, NOLABEL, looptoplabel, loopendlabel, n->op);
  genfreeregs(NOREG);

  // If there is an optional ELSE clause,
  // generate the jump to skip to the end
  if (nright)
    cgjump(Lend);

  // Now the false label
  cglabel(Lfalse);

  // Optional ELSE clause: generate the
  // false compound statement and the
  // end label
  if (nright) {
    genAST(nright, NOLABEL, looptoplabel, loopendlabel, n->op);
    genfreeregs(NOREG);
    cglabel(Lend);
  }

  return (NOREG);
}

// Generate the code for a WHILE statement
static int genWHILE(struct ASTnode *n, struct ASTnode *nleft,
		    struct ASTnode *nright) {
  int Lstart, Lend;

  // Generate the start and end labels
  // and output the start label
  Lstart = genlabel();
  Lend = genlabel();
  cglabel(Lstart);

  // Generate the condition code followed
  // by a jump to the end label.
  genAST(nleft, Lend, Lstart, Lend, n->op);
  genfreeregs(NOREG);

  // Generate the compound statement for the body
  genAST(nright, NOLABEL, Lstart, Lend, n->op);
  genfreeregs(NOREG);

  // Finally output the jump back to the condition,
  // and the end label
  cgjump(Lstart);
  cglabel(Lend);
  return (NOREG);
}

// Generate the code for a SWITCH statement
static int genSWITCH(struct ASTnode *n, int looptoplabel) {
  int *caseval, *caselabel;
  int Ljumptop, Lend;
  int i, reg, defaultlabel = 0, casecount = 0;
  int rightid;
  struct ASTnode *nleft;
  struct ASTnode *nright;
  struct ASTnode *c, *cleft;

  // Load in the sub-nodes
  nleft=loadASTnode(n->leftid,0);
  nright=loadASTnode(n->rightid,0);

  // Create arrays for the case values and associated labels.
  // Ensure that we have at least one position in each array.
  caseval = (int *) malloc((n->a_intvalue + 1) * sizeof(int));
  caselabel = (int *) malloc((n->a_intvalue + 1) * sizeof(int));

  // Generate labels for the top of the jump table, and the
  // end of the switch statement. Set a default label for
  // the end of the switch, in case we don't have a default.
  Ljumptop = genlabel();
  Lend = genlabel();
  defaultlabel = Lend;

  // Build the case value and label arrays
  for (i = 0, c = nright; c != NULL;) {
    // Get a label for this case. Store it
    // and the case value in the arrays.
    // Record if it is the default case.
    caselabel[i] = genlabel();
    caseval[i] = c->a_intvalue;
    if (c->op == A_DEFAULT)
      defaultlabel = caselabel[i];
    else
      casecount++;

    i++;
    rightid= c->rightid;
    // Don't free c if it is nright,
    // as genAST() will do this
    if (c!=nright) freeASTnode(c);
    c = loadASTnode(rightid,0);
  }

  // Output the code to calculate the switch condition
  reg = genAST(nleft, NOLABEL, NOLABEL, NOLABEL, 0);
  cgjump(Ljumptop);
  genfreeregs(reg);

  // Output the switch code or switch table
  cgswitch(reg, casecount, Ljumptop, caselabel, caseval, defaultlabel);

  // Generate the code for each case
  for (i = 0, c = nright; c != NULL; ) {
    // Generate the case code. Pass in the end label for the breaks.
    // If case has no body, we will fall into the following body.
    cglabel(caselabel[i]);
    if (c->leftid) {
      // Looptoplabel is here so we can 'continue', e.g.
      // while (...) {
      //   switch(...) {
      //     case ...: ...
      //               continue;
      //   }
      // }
      cleft= loadASTnode(c->leftid,0);
      genAST(cleft, NOLABEL, looptoplabel, Lend, 0);
      freeASTnode(cleft);
    }
    genfreeregs(NOREG);

    i++;
    rightid= c->rightid;
    freeASTnode(c);
    c= loadASTnode(rightid,0);
  }

  // Output the end label
  cglabel(Lend);
  free(caseval);
  free(caselabel);
  freeASTnode(nleft);
  return (NOREG);
}

// Generate the code for an A_LOGOR operation.
// If the parent AST node is an A_IF, A_WHILE, A_TERNARY
// or A_LOGAND, jump to the label if false.
// If A_LOGOR, jump to the label if true.
// Otherwise set a register to 1 or 0 and return it.
static int gen_logor(struct ASTnode *n,
		     struct ASTnode *nleft, struct ASTnode *nright,
		     int parentASTop, int label) {
  int Ltrue, Lfalse, Lend;
  int reg;
  int type;
  int makebool = 0;

  // Generate labels
  if (parentASTop == A_LOGOR) {
    Ltrue = label;
    Lfalse = genlabel();
  } else {
    Ltrue = genlabel();
    Lfalse = label;
  }
  Lend = genlabel();

  // Mark if we need to generate a boolean value
  if (parentASTop != A_IF && parentASTop != A_WHILE &&
      parentASTop != A_TERNARY && parentASTop != A_LOGAND &&
      parentASTop != A_LOGOR) {
    makebool = 1;
    Ltrue = genlabel();
    Lfalse = genlabel();
  }

  // Generate the code for the left expression.
  // The genAST() could do the jump and return NOREG.
  // But if we get a register back, do our own jump.
  reg = genAST(nleft, Ltrue, NOLABEL, NOLABEL, A_LOGOR);
  if (reg != NOREG) {
    type = nleft->type;
    cgboolean(reg, A_LOGOR, Ltrue, type);
    genfreeregs(NOREG);
  }

  // Generate the code for the right expression
  // with the same logic as for the left expression.
  reg = genAST(nright, Ltrue, NOLABEL, NOLABEL, A_LOGOR);
  if (reg != NOREG) {
    type = nright->type;
    cgboolean(reg, A_LOGOR, Ltrue, type);
    genfreeregs(reg);
  }

  // The result is false.
  // If there is no need to make a boolean, stop now
  if (makebool == 0) {
    // Jump to the false label if it was provided
    if (label == Lfalse) {
      cgjump(Lfalse);
      cglabel(Ltrue);
    }
    return (NOREG);
  }

  // We do need to make a boolean and we didn't jump
  type = n->type;
  cglabel(Lfalse);
  reg = cgloadboolean(reg, 0, type);
  cgjump(Lend);
  cglabel(Ltrue);
  reg = cgloadboolean(reg, 1, type);
  cglabel(Lend);
  return (reg);
}

// Generate the code for an A_LOGAND operation.
// If the parent AST node is an A_IF, A_WHILE, A_TERNARY
// or A_LOGAND, jump to the label if false.
// If A_LOGOR, jump to the label if true.
// Otherwise set a register to 1 or 0 and return it.
static int gen_logand(struct ASTnode *n,
		      struct ASTnode *nleft, struct ASTnode *nright,
		      int parentASTop, int label) {
  int Ltrue, Lfalse, Lend;
  int reg;
  int type;
  int makebool = 0;

  // Generate labels
  if (parentASTop == A_LOGOR) {
    Ltrue = label;
    Lfalse = genlabel();
  } else {
    Ltrue = genlabel();
    Lfalse = label;
  }

  Lend = genlabel();

  // Mark if we need to generate a boolean value
  if (parentASTop != A_IF && parentASTop != A_WHILE &&
      parentASTop != A_TERNARY && parentASTop != A_LOGAND &&
      parentASTop != A_LOGOR) {
    makebool = 1;
    Ltrue = genlabel();
    Lfalse = genlabel();
  }

  // Generate the code for the left expression.
  // The genAST() could do the jump and return NOREG.
  // But if we get a register back, do our own jump.
  reg = genAST(nleft, Lfalse, NOLABEL, NOLABEL, A_LOGAND);
  if (reg != NOREG) {
    type = nleft->type;
    cgboolean(reg, A_LOGAND, Lfalse, type);
    genfreeregs(NOREG);
  }

  // Generate the code for the right expression
  // with the same logic as for the left expression.
  reg = genAST(nright, Lfalse, NOLABEL, NOLABEL, A_LOGAND);
  if (reg != NOREG) {
    type = nright->type;
    cgboolean(reg, A_LOGAND, Lfalse, type);
    genfreeregs(reg);
  }

  // The result is true.
  // If there is no need to make a boolean, stop now
  if (makebool == 0) {
    // Jump to the label if we were given it
    if (label == Ltrue) {
      cgjump(Ltrue);
      cglabel(Lfalse);
    }
    return (NOREG);
  }

  // We do need to make a boolean and we didn't jump
  type = n->type;
  cglabel(Ltrue);
  reg = cgloadboolean(reg, 1, type);
  cgjump(Lend);
  cglabel(Lfalse);
  reg = cgloadboolean(reg, 0, type);
  cglabel(Lend);
  return (reg);
}

// Generate the code to calculate the arguments of a
// function call, then call the function with these
// arguments. Return the register that holds
// the function's return value.
static int gen_funccall(struct ASTnode *n) {
  struct ASTnode *gluetree;
  int i = 0, numargs = 0;
  int reg;
  int leftid;
  int *arglist = NULL;
  int *typelist = NULL;
  struct ASTnode *nleft;
  struct ASTnode *glueright;

  // Load in the sub-nodes
  nleft=loadASTnode(n->leftid,0);

  // Determine the actual number of arguments
  // Allocate memory to hold the list of argument temporaries.
  // We need to walk the list of arguments to determine the size
  // XXX We need to free here
  for (i = 0, gluetree = nleft; gluetree != NULL; ) {
    numargs++;
    i++;
    leftid= gluetree->leftid;
    // Don't free gluetree if it is nleft,
    // as genAST() will do this for us
    if (gluetree != nleft) freeASTnode(gluetree);
    gluetree = loadASTnode(leftid,0);
  }

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
  for (i = 0, gluetree = nleft; gluetree != NULL;
				gluetree = loadASTnode(leftid,0)) {
    // Calculate the expression's value
    glueright= loadASTnode(gluetree->rightid,0);
    arglist[i] =
      genAST(glueright, NOLABEL, NOLABEL, NOLABEL, gluetree->op);
    typelist[i++] = glueright->type;
    freeASTnode(glueright);
    leftid= gluetree->leftid;
    freeASTnode(gluetree);
  }

  // Call the function and return its result
  reg= cgcall(n->sym, numargs, arglist, typelist);
  free(arglist);
  free(typelist);
  return(reg);
}

// Generate code for a ternary expression
static int gen_ternary(struct ASTnode *n, struct ASTnode *nleft,
		       struct ASTnode *nmid, struct ASTnode *nright) {
  int Lfalse, Lend;
  int reg, expreg;

  // Load in the sub-nodes
  nleft=loadASTnode(n->leftid,0);
  nmid=loadASTnode(n->midid,0);
  nright=loadASTnode(n->rightid,0);

  // Generate two labels: one for the
  // false expression, and one for the
  // end of the overall expression
  Lfalse = genlabel();
  Lend = genlabel();

  // Generate the condition code followed
  // by a jump to the false label.
  genAST(nleft, Lfalse, NOLABEL, NOLABEL, n->op);
  // genfreeregs(NOREG);

  // Get a register to hold the result of the two expressions
  reg = cgallocreg(nleft->type);

  // Generate the true expression and the false label.
  // Move the expression result into the known register.
  expreg = genAST(nmid, NOLABEL, NOLABEL, NOLABEL, n->op);
  cgmove(expreg, reg, nmid->type);
  cgfreereg(expreg);
  cgjump(Lend);
  cglabel(Lfalse);

  // Generate the false expression and the end label.
  // Move the expression result into the known register.
  expreg = genAST(nright, NOLABEL, NOLABEL, NOLABEL, n->op);
  cgmove(expreg, reg, nright->type);
  cgfreereg(expreg);
  cglabel(Lend);
  return (reg);
}

// Given an AST, an optional label, and the AST op
// of the parent, generate assembly code recursively.
// Return the register id with the tree's final value.
int genAST(struct ASTnode *n, int iflabel, int looptoplabel,
	   int loopendlabel, int parentASTop) {
  int leftreg = NOREG, rightreg = NOREG;
  int type = P_VOID;
  int id;
  int special = 0;
  struct ASTnode *nleft, *nmid, *nright;

  // Empty tree, do nothing
  if (n == NULL)
    return (NOREG);

  // Load in the sub-nodes
  nleft=loadASTnode(n->leftid,0);
  nmid=loadASTnode(n->midid,0);
  nright=loadASTnode(n->rightid,0);

  // Update the line number in the output
  update_line(n);

  // We have some specific AST node handling at the top
  // so that we don't evaluate the child sub-trees immediately
  switch (n->op) {
  case A_IF:
    special = 1;
    leftreg = genIF(n, nleft, nmid, nright, looptoplabel, loopendlabel);
    break;
  case A_WHILE:
    special = 1;
    leftreg = genWHILE(n, nleft, nright);
    break;
  case A_SWITCH:
    special = 1;
    leftreg = genSWITCH(n, looptoplabel);
    break;
  case A_FUNCCALL:
    special = 1;
    leftreg = gen_funccall(n);
    break;
  case A_TERNARY:
    special = 1;
    leftreg = gen_ternary(n, nleft, nmid, nright);
    break;
  case A_LOGOR:
    special = 1;
    leftreg = gen_logor(n, nleft, nright, parentASTop, iflabel);
    break;
  case A_LOGAND:
    special = 1;
    leftreg = gen_logand(n, nleft, nright, parentASTop, iflabel);
    break;
  case A_GLUE:
    // Do each child statement, and free the
    // registers after each child
    special = 1;
    if (nleft != NULL)
      genAST(nleft, iflabel, looptoplabel, loopendlabel, n->op);
    genfreeregs(NOREG);
    if (nright != NULL)
      genAST(nright, iflabel, looptoplabel, loopendlabel, n->op);
    genfreeregs(NOREG);
    leftreg = NOREG;
    break;
  case A_FUNCTION:
    // Generate the function's preamble before the code
    // in the child sub-tree. Ugly: use function's name
    // as the Infilename for fatal messages.
    special = 1;
    Infilename = n->sym->name;
    cgfuncpreamble(n->sym);
    genAST(nleft, NOLABEL, NOLABEL, NOLABEL, n->op);
    cgfuncpostamble(n->sym);
    leftreg = NOREG;
  }

  if (!special) {

    // General AST node handling below

    // Get the left and right sub-tree values
    if (nleft) {
      type = nleft->type;
      leftreg = genAST(nleft, NOLABEL, looptoplabel, loopendlabel, n->op);
    }
    if (nright) {
      type = nright->type;
      rightreg = genAST(nright, NOLABEL, looptoplabel, loopendlabel, n->op);
    }

    switch (n->op) {
    case A_ADD:
      leftreg = cgadd(leftreg, rightreg, type);
      break;
    case A_SUBTRACT:
      leftreg = cgsub(leftreg, rightreg, type);
      break;
    case A_MULTIPLY:
      leftreg = cgmul(leftreg, rightreg, type);
      break;
    case A_DIVIDE:
      leftreg = cgdiv(leftreg, rightreg, type);
      break;
    case A_MOD:
      leftreg = cgmod(leftreg, rightreg, type);
      break;
    case A_AND:
      leftreg = cgand(leftreg, rightreg, type);
      break;
    case A_OR:
      leftreg = cgor(leftreg, rightreg, type);
      break;
    case A_XOR:
      leftreg = cgxor(leftreg, rightreg, type);
      break;
    case A_LSHIFT:
      leftreg = cgshl(leftreg, rightreg, type);
      break;
    case A_RSHIFT:
      leftreg = cgshr(leftreg, rightreg, type);
      break;
    case A_EQ:
    case A_NE:
    case A_LT:
    case A_GT:
    case A_LE:
    case A_GE:
      // If the parent AST node is an A_IF, A_WHILE, A_TERNARY,
      // A_LOGAND, generate a compare followed by a jump if the
      // comparison is false. If A_LOGOR, jump if true. Otherwise,
      // compare registers and set one to 1 or 0 based on the comparison.
      if (parentASTop == A_IF || parentASTop == A_WHILE ||
	  parentASTop == A_TERNARY || parentASTop == A_LOGAND ||
	  parentASTop == A_LOGOR) {
	leftreg = cgcompare_and_jump
	  (n->op, parentASTop, leftreg, rightreg, iflabel, nleft->type);
      } else {
	leftreg = cgcompare_and_set(n->op, leftreg, rightreg, nleft->type);
      }
      break;
    case A_INTLIT:
      leftreg = cgloadint(n->a_intvalue, n->type);
      break;
    case A_STRLIT:
      // Output the actual literal
      id = genglobstr(n->name);
      leftreg = cgloadglobstr(id);
      break;
    case A_IDENT:
      // Load our value if we are an rvalue
      // or we are being dereferenced
      if (n->rvalue || parentASTop == A_DEREF) {
	leftreg = cgloadvar(n->sym, n->op);
      } else
	leftreg = NOREG;
      break;
    case A_ASPLUS:
    case A_ASMINUS:
    case A_ASSTAR:
    case A_ASSLASH:
    case A_ASMOD:
    case A_ASSIGN:

      // For the '+=' and friends operators, generate suitable code
      // and get the register with the result. Then take the left child,
      // make it the right child so that we can fall into the assignment code.
      switch (n->op) {
      case A_ASPLUS:
	leftreg = cgadd(leftreg, rightreg, type);
	nright = nleft;
	break;
      case A_ASMINUS:
	leftreg = cgsub(leftreg, rightreg, type);
	nright = nleft;
	break;
      case A_ASSTAR:
	leftreg = cgmul(leftreg, rightreg, type);
	nright = nleft;
	break;
      case A_ASSLASH:
	leftreg = cgdiv(leftreg, rightreg, type);
	nright = nleft;
	break;
      case A_ASMOD:
	leftreg = cgmod(leftreg, rightreg, type);
	nright = nleft;
	break;
      }

      // Now into the assignment code
      // Are we assigning to an identifier or through a pointer?
      switch (nright->op) {
      case A_IDENT:
	if (nright->sym->class == V_GLOBAL ||
	    nright->sym->class == V_EXTERN ||
	    nright->sym->class == V_STATIC) {
	  leftreg = cgstorglob(leftreg, nright->sym);
	} else {
	  leftreg = cgstorlocal(leftreg, nright->sym);
	}
	break;
      case A_DEREF:
	leftreg = cgstorderef(leftreg, rightreg, nright->type);
	break;
      default:
	fatald("Can't A_ASSIGN in genAST(), op", n->op);
      }
      break;
    case A_WIDEN:
      // Widen the child's type to the parent's type
      leftreg = cgwiden(leftreg, nleft->type, n->type);
      break;
    case A_RETURN:
      cgreturn(leftreg, Functionid);
      leftreg = NOREG;
      break;
    case A_ADDR:
      // If we have a symbol, get its address. Otherwise,
      // the left register already has the address because
      // it's a member access
      if (n->sym != NULL)
	leftreg = cgaddress(n->sym);
      break;
#ifdef SPLITSWITCH
      }

      // I've broken the switch statement into two, so that
      // the 6809 version of the compiler can parse this file
      // without running out of room.
    switch (n->op) {
#endif
    case A_DEREF:
      // If we are an rvalue, dereference to get the value we point at,
      // otherwise leave it for A_ASSIGN to store through the pointer
      if (n->rvalue)
	leftreg = cgderef(leftreg, nleft->type);
      break;
    case A_SCALE:
      // Small optimisation: use shift if the
      // scale value is a known power of two
      switch (n->a_size) {
      case 2:
	leftreg = cgshlconst(leftreg, 1, type);
	break;
      case 4:
	leftreg = cgshlconst(leftreg, 2, type);
	break;
      case 8:
	leftreg = cgshlconst(leftreg, 3, type);
	break;
      default:
	// Load a register with the size and
	// multiply the leftreg by this size
	rightreg = cgloadint(n->a_size, P_INT);
	leftreg = cgmul(leftreg, rightreg, type);
      }

      // On some architectures the pointer type is
      // different to the int type. Widen the result
      // if we are scaling what will become an address offset
      if (cgprimsize(n->type) > cgprimsize(type))
	leftreg = cgwiden(leftreg, type, n->type);

      break;
    case A_POSTINC:
    case A_POSTDEC:
      // Load and decrement the variable's value into a register
      // and post increment/decrement it
      leftreg = cgloadvar(n->sym, n->op);
      break;
    case A_PREINC:
    case A_PREDEC:
      // Load and decrement the variable's value into a register
      // and pre increment/decrement it
      leftreg = cgloadvar(nleft->sym, n->op);
      break;
    case A_NEGATE:
      leftreg = cgnegate(leftreg, type);
      break;
    case A_INVERT:
      leftreg = cginvert(leftreg, type);
      break;
    case A_LOGNOT:
      leftreg = cglognot(leftreg, type);
      break;
    case A_TOBOOL:
      // If the parent AST node is an IF, WHILE, TERNARY,
      // LOGAND or LOGOR operation, generate a compare
      // followed by a jump. Otherwise, set the register
      // to 0 or 1 based on it's zeroeness or non-zeroeness
      leftreg = cgboolean(leftreg, parentASTop, iflabel, type);
      break;
    case A_BREAK:
      cgjump(loopendlabel);
      leftreg = NOREG;
      break;
    case A_CONTINUE:
      cgjump(looptoplabel);
      leftreg = NOREG;
      break;
    case A_CAST:
      leftreg = cgcast(leftreg, nleft->type, n->type);
      break;
#ifndef SPLITSWITCH
    default:
      fatald("Unknown AST operator", n->op);
#endif
    }
  }				// End of if (!special)

  // Free the AST sub trees before returning
  // Sometimes n->right has been set to n->left
  // e.g. by the +=, -= etc. operations.
  if (nright != nleft)
    freeASTnode(nright);
  freeASTnode(nleft);
  freeASTnode(nmid);
  return (leftreg);
}

void genpreamble() {
  cgpreamble();
}

void genpostamble() {
  cgpostamble();
}

void genglobsym(struct symtable *node) {
  cgglobsym(node);
}

// Generate a global string.
int genglobstr(char *strvalue) {
  int l = genlabel();
  cglitseg();
  cgglobstr(l, strvalue);
  cgtextseg();
  return (l);
}
