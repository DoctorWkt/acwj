#include "defs.h"
#include "data.h"
#include "decl.h"

// AST tree functions
// Copyright (c) 2019 Warren Toomey, GPL3

// Build and return a generic AST node
struct ASTnode *mkastnode(int op, int type,
			  struct symtable *ctype,
			  struct ASTnode *left,
			  struct ASTnode *mid,
			  struct ASTnode *right,
			  struct symtable *sym, int intvalue) {
  struct ASTnode *n;

  // Malloc a new ASTnode
  n = (struct ASTnode *) malloc(sizeof(struct ASTnode));
  if (n == NULL)
    fatal("Unable to malloc in mkastnode()");

  // Copy in the field values and return it
  n->op = op;
  n->type = type;
  n->ctype = ctype;
  n->left = left;
  n->mid = mid;
  n->right = right;
  n->sym = sym;
  n->a_intvalue = intvalue;
  n->linenum = 0;
  return (n);
}


// Make an AST leaf node
struct ASTnode *mkastleaf(int op, int type,
			  struct symtable *ctype,
			  struct symtable *sym, int intvalue) {
  return (mkastnode(op, type, ctype, NULL, NULL, NULL, sym, intvalue));
}

// Make a unary AST node: only one child
struct ASTnode *mkastunary(int op, int type,
			   struct symtable *ctype,
			   struct ASTnode *left,
			   struct symtable *sym, int intvalue) {
  return (mkastnode(op, type, ctype, left, NULL, NULL, sym, intvalue));
}

// Generate and return a new label number
// just for AST dumping purposes
static int dumpid = 1;
static int gendumplabel(void) {
  return (dumpid++);
}

// List of AST node names
static char *astname[] = { NULL,
  "ASSIGN", "ASPLUS", "ASMINUS", "ASSTAR",
  "ASSLASH", "ASMOD", "TERNARY", "LOGOR",
  "LOGAND", "OR", "XOR", "AND", "EQ", "NE", "LT",
  "GT", "LE", "GE", "LSHIFT", "RSHIFT",
  "ADD", "SUBTRACT", "MULTIPLY", "DIVIDE", "MOD",
  "INTLIT", "STRLIT", "IDENT", "GLUE",
  "IF", "WHILE", "FUNCTION", "WIDEN", "RETURN",
  "FUNCCALL", "DEREF", "ADDR", "SCALE",
  "PREINC", "PREDEC", "POSTINC", "POSTDEC",
  "NEGATE", "INVERT", "LOGNOT", "TOBOOL", "BREAK",
  "CONTINUE", "SWITCH", "CASE", "DEFAULT", "CAST"
};

// Given an AST tree, print it out and follow the
// traversal of the tree that genAST() follows
void dumpAST(struct ASTnode *n, int label, int level) {
  int Lfalse, Lstart, Lend;
  int i;

  if (n == NULL)
    fatal("NULL AST node");
  if (n->op > A_CAST)
    fatald("Unknown dumpAST operator", n->op);

  // Deal with IF and WHILE statements specifically
  switch (n->op) {
    case A_IF:
      Lfalse = gendumplabel();
      for (i = 0; i < level; i++)
	fprintf(stdout, " ");
      fprintf(stdout, "IF");
      if (n->right) {
	Lend = gendumplabel();
	fprintf(stdout, ", end L%d", Lend);
      }
      fprintf(stdout, "\n");
      dumpAST(n->left, Lfalse, level + 2);
      dumpAST(n->mid, NOLABEL, level + 2);
      if (n->right)
	dumpAST(n->right, NOLABEL, level + 2);
      return;
    case A_WHILE:
      Lstart = gendumplabel();
      for (i = 0; i < level; i++)
	fprintf(stdout, " ");
      fprintf(stdout, "WHILE, start L%d\n", Lstart);
      Lend = gendumplabel();
      dumpAST(n->left, Lend, level + 2);
      if (n->right)
	dumpAST(n->right, NOLABEL, level + 2);
      return;
  }

  // Reset level to -2 for A_GLUE nodes
  if (n->op == A_GLUE) {
    level -= 2;
  } else {

    // General AST node handling
    for (i = 0; i < level; i++)
      fprintf(stdout, " ");
    fprintf(stdout, "%s", astname[n->op]);
    switch (n->op) {
      case A_FUNCTION:
      case A_FUNCCALL:
      case A_ADDR:
      case A_PREINC:
      case A_PREDEC:
	if (n->sym != NULL)
	  fprintf(stdout, " %s", n->sym->name);
	break;
      case A_INTLIT:
	fprintf(stdout, " %d", n->a_intvalue);
	break;
      case A_STRLIT:
	fprintf(stdout, " rval label L%d", n->a_intvalue);
	break;
      case A_IDENT:
	if (n->rvalue)
	  fprintf(stdout, " rval %s", n->sym->name);
	else
	  fprintf(stdout, " %s", n->sym->name);
	break;
      case A_DEREF:
	if (n->rvalue)
	  fprintf(stdout, " rval");
	break;
      case A_SCALE:
	fprintf(stdout, " %d", n->a_size);
	break;
      case A_CASE:
	fprintf(stdout, " %d", n->a_intvalue);
	break;
      case A_CAST:
	fprintf(stdout, " %d", n->type);
	break;
    }
    fprintf(stdout, "\n");
  }

  // General AST node handling
  if (n->left)
    dumpAST(n->left, NOLABEL, level + 2);
  if (n->mid)
    dumpAST(n->mid, NOLABEL, level + 2);
  if (n->right)
    dumpAST(n->right, NOLABEL, level + 2);
}
