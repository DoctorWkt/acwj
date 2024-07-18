#include "defs.h"
#define extern_
#include "data.h"
#undef extern_
#include "misc.h"
#include "tree.h"

// Deserialise an AST
// Copyright (c) 2023 Warren Toomey, GPL3

int showglue=0;

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

// Given an AST node, print it out and then
// recursively deal with the sub-nodes.
void dumpAST(struct ASTnode *n, int label, int level) {
  int Lfalse, Lstart, Lend;
  int i;
  struct ASTnode *nleft=NULL, *nmid=NULL, *nright=NULL;

  if (n == NULL)
    fatal("NULL AST node");
  if (n->op > A_CAST)
    fatald("Unknown dumpAST operator", n->op);

  // Load in the sub-nodes
  if (n->leftid) nleft=loadASTnode(n->leftid,0);
  if (n->midid) nmid=loadASTnode(n->midid,0);
  if (n->rightid) nright=loadASTnode(n->rightid,0);

  // Deal with IF and WHILE statements specifically
  switch (n->op) {
    case A_IF:
      Lfalse = gendumplabel();
      for (i = 0; i < level; i++)
	fprintf(stdout, " ");
      fprintf(stdout, "IF");
      if (nright) {
	Lend = gendumplabel();
	fprintf(stdout, ", end L%d", Lend);
      }
      fprintf(stdout, " (id %d)\n", n->nodeid);
      dumpAST(nleft, Lfalse, level + 2);
      dumpAST(nmid, NOLABEL, level + 2);
      if (nright)
	dumpAST(nright, NOLABEL, level + 2);
      free(n);
      return;
    case A_WHILE:
      Lstart = gendumplabel();
      for (i = 0; i < level; i++)
	fprintf(stdout, " ");
      fprintf(stdout, "WHILE start L%d (id %d)\n", Lstart, n->nodeid);
      Lend = gendumplabel();
      dumpAST(nleft, Lend, level + 2);
      if (nright)
	dumpAST(nright, NOLABEL, level + 2);
      free(n);
      return;
  }

  // Reset level to -2 for A_GLUE nodes
  if (n->op == A_GLUE) {
    if (showglue) fprintf(stdout, "glue %d %d\n", n->leftid, n->rightid);
    level -= 2;
  } else {

    // General AST node handling
    for (i = 0; i < level; i++)
      fprintf(stdout, " ");
    fprintf(stdout, "%s", astname[n->op]);
    if (n->symid != 0)
      fprintf(stdout, " symid %d", n->symid);
    switch (n->op) {
      case A_FUNCTION:
      case A_FUNCCALL:
      case A_ADDR:
      case A_PREINC:
      case A_POSTINC:
	if (n->name != NULL)
	  fprintf(stdout, " %s", n->name);
	break;
      case A_INTLIT:
	fprintf(stdout, " %d", n->a_intvalue);
	break;
      case A_STRLIT:
	fprintf(stdout, " rval \"%s\"", n->name);
	break;
      case A_IDENT:
	if (n->name != NULL) {
	  if (n->rvalue)
	    fprintf(stdout, " rval %s", n->name);
	  else
	    fprintf(stdout, " %s", n->name);
	}
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
    fprintf(stdout, " (id %d)\n", n->nodeid);
  }

  // General AST node handling
  if (nleft) dumpAST(nleft, NOLABEL, level + 2);
  if (nmid) dumpAST(nmid, NOLABEL, level + 2);
  if (nright) dumpAST(nright, NOLABEL, level + 2);
  if (n->name!=NULL) free(n->name);
  free(n);
}

int main(int argc, char **argv) {
  struct ASTnode *node;
  int fileid= 1;

  if (argc !=2 && argc!=3) {
    fprintf(stderr, "Usage: %s [-g] astfile\n", argv[0]); exit(1);
  }

  if (!strcmp(argv[1], "-g")) {
   showglue=1; fileid=2;
  }

  Infile= fopen(argv[fileid], "r");
  if (Infile==NULL) {
    fprintf(stderr, "Unable to open %s\n", argv[fileid]); exit(1);
  }

  Idxfile= tmpfile();
  mkASTidxfile();               // Build the AST index offset file

  // Loop reading the next function's top node in from file
  while (1) {
    node= loadASTnode(0, 1);
    if (node==NULL) break;

    // Dump the function's tree
    dumpAST(node, NOLABEL, 0);
    printf("\n\n");
  }
  return (0);
}
