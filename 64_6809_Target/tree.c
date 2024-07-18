#include "defs.h"
#include "data.h"
#include "misc.h"
#include "parse.h"
#include "sym.h"
#include "gen.h"

// AST tree functions
// Copyright (c) 2019,2024 Warren Toomey, GPL3

#undef DEBUG

// Used to enumerate the AST nodes
static int nodeid= 1;

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
  n->nodeid= nodeid++;
  n->op = op;
  n->type = type;
  n->ctype = ctype;
  n->left = left;
  n->mid = mid;
  n->right = right;
  n->leftid= 0;
  n->midid= 0;
  n->rightid= 0;
#ifdef DEBUG
fprintf(stderr, "mkastnodeA l %d m %d r %d\n", n->leftid, n->midid, n->rightid);
#endif
  if (left!=NULL) n->leftid= left->nodeid;
  if (mid!=NULL) n->midid= mid->nodeid;
  if (right!=NULL) n->rightid= right->nodeid;
#ifdef DEBUG
fprintf(stderr, "mkastnodeB l %d m %d r %d\n", n->leftid, n->midid, n->rightid);
#endif
  n->sym = sym;
  if (sym != NULL) {
    n->name= sym->name;
    n->symid= sym->id;
  } else {
    n->name= NULL;
    n->symid= 0;
  }
  n->a_intvalue = intvalue;
  n->linenum = 0;
  n->rvalue = 0;
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

// Free the given AST node
void freeASTnode(struct ASTnode *tree) {
  if (tree==NULL) return;
  if (tree->name != NULL) free(tree->name);
  free(tree);
}

// Free the contents of a tree. Possibly
// because of tree optimisation, sometimes
// left and right are the same sub-nodes.
// Free the names if asked to do so.
void freetree(struct ASTnode *tree, int freenames) {
  if (tree==NULL) return;

  if (tree->left!=NULL) freetree(tree->left, freenames);
  if (tree->mid!=NULL) freetree(tree->mid, freenames);
  if (tree->right!=NULL && tree->right!=tree->left)
					freetree(tree->right, freenames);
  if (freenames && tree->name != NULL) free(tree->name);
  free(tree);
}

#ifndef WRITESYMS

// We record the id of the last function that we loaded.
// and the highest index in the array below
static int lastFuncid= -1;
static int hiFuncid;

// We also keep an array of AST node offsets that
// represent the functions in the AST file
long *Funcoffset;

// Given an AST node id, load that AST node from the AST file.
// If nextfunc is set, find the next AST node which is a function.
// Allocate and return the node or NULL if it can't be found.
struct ASTnode *loadASTnode(int id, int nextfunc) {
  long offset, idxoff;
  struct ASTnode *node;

  // Do nothing if nothing to do
  if (id==0 && nextfunc==0) return(NULL);

#ifdef DEBUG
  fprintf(stderr, "loadASTnode id %d nextfunc %d\n", id, nextfunc);

  if (id < 0)
    fatal("negative id in loadASTnode()");
#endif

  // Determine the offset of the node.
  // Use the function offset array, or
  // use the AST index file otherwise
  if (nextfunc==1) {
    lastFuncid++;
    if (lastFuncid > hiFuncid)
      return(NULL);
    offset= Funcoffset[lastFuncid];
  } else {
    idxoff= id * sizeof(long);
    fseek(Idxfile, idxoff, SEEK_SET);
    fread(&offset, sizeof(long), 1, Idxfile);
  }

  // Allocate a node
  node= (struct ASTnode *)malloc(sizeof(struct ASTnode));
  if (node==NULL)
    fatal("Cannot malloc an AST node in loadASTnode");

  // Read the node in from the AST file. Give up if EOF
  fseek(Infile, offset, SEEK_SET);
  if (fread(node, sizeof(struct ASTnode), 1, Infile)!=1) {
    free(node); return(NULL);
  }

#ifdef DEBUG
  // Check that the node we loaded was the one we wanted
  if (id!=0 && id!=node->nodeid)
    fprintf(stderr, "Wanted AST node id %d, got %d\n", id, node->nodeid);
#endif

  // If there is a string/identifier literal, get it
  if (node->name!=NULL) {
    fgetstr(Text, TEXTLEN + 1, Infile);
    node->name= strdup(Text);
    if (node->name==NULL)
      fatal("Unable to malloc string literal in deserialiseAST()");

#ifndef DETREE
    // If this wasn't a string literal
    // search for the actual symbol and link it in
    if (node->op != A_STRLIT) {
      node->sym= findSymbol(NULL, 0, node->symid);
      if (node->sym==NULL)
        fatald("Can't find symbol with id", node->symid);
    }
#endif
  }

  // Set the pointers to NULL to trip us up!
  node->left= node->mid= node->right= NULL;

#ifndef DETREE
  // If this is a function, set the global
  // Functionid and create an endlabel for it.
  // Update the lastFuncnode too.
  if (node->op== A_FUNCTION) {
    Functionid= node->sym;
    Functionid->st_endlabel= genlabel();
  }
#endif

  // Return the node that we found
#ifdef DEBUG
  fprintf(stderr, "Found AST node id %d\n", node->nodeid);
#endif
  return(node);
}

// Using the open AST file and the newly-created
// index file, build a list of AST file offsets
// for each AST node in the AST file.
void mkASTidxfile(void) {
  struct ASTnode *node;
  long offset, idxoff;

  // Allocate a node and at least some Funcoffset area
  node= (struct ASTnode *)malloc(sizeof(struct ASTnode));
  Funcoffset= (long *)malloc(sizeof(long));
  if (node==NULL || Funcoffset==NULL)
    fatal("Cannot malloc an AST node in loadASTnode");

  while (1) {
    // Get the current offset
    offset = ftell(Infile);
#ifdef DEBUG
    if (sizeof(long)==4)
      fprintf(stderr, "A offset %ld sizeof ASTnode %d\n", offset,
			sizeof(struct ASTnode));
    else
      fprintf(stderr, "A offset %ld sizeof ASTnode %ld\n", offset,
			sizeof(struct ASTnode));
#endif

    // Read in the next node, stop if none
    if (fread(node, sizeof(struct ASTnode), 1, Infile)!=1) {
      break;
    }
#ifdef DEBUG
    fprintf(stderr, "Node %d at offset %ld\n", node->nodeid, offset);
    fprintf(stderr, "Node %d left %d mid %d right %d\n", node->nodeid,
        node->leftid, node->midid, node->rightid);
#endif

    // If there is a string/identifier literal, get it
    if (node->name!=NULL) {
      fgetstr(Text, TEXTLEN + 1, Infile);
#ifdef DEBUG
    fprintf(stderr, "  name %s\n", Text);
#endif
    }
    
    // Save the node's offset at its index position in the file.
    idxoff= node->nodeid * sizeof(long);

    fseek(Idxfile, idxoff, SEEK_SET);
    fwrite(&offset, sizeof(long), 1, Idxfile);

    // If this node is a function, increase the size
    // of the function index array and save the offset
    if (node->op==A_FUNCTION) {
      lastFuncid++;
      Funcoffset= (long *)realloc(Funcoffset, sizeof(long)* (lastFuncid+1));
      Funcoffset[lastFuncid]= offset;
    }
  }

  // Reset before we start using the array
  hiFuncid= lastFuncid; lastFuncid= -1;
  free(node);
}
#endif // WRITESYMS
