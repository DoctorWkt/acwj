#include "defs.h"
#define extern_
#include "data.h"
#undef extern_
#include "gen.h"
#include "misc.h"
#include "sym.h"
#include "tree.h"
#include "types.h"

// Assembly code generator.
// Copyright (c) 2023,2024 Warren Toomey, GPL3

// Allocate space for the variables
// then free the symbol table.
void allocateGlobals(void) {
  struct symtable *sym, *litsym;
  int i;

  // Load all the types and all the globals
  loadGlobals();

  // We now have all the types and all the globals in memory
  // Generate the string literals first
  for (sym=Symhead; sym!=NULL; sym=sym->next) {
      if (sym->stype== S_STRLIT)
 	sym->st_label= genglobstr(sym->name);
  }

  // Now do the non string literals
  // XXX To fix: sym=sym->next
  for (sym=Symhead; sym!=NULL; ) {
      if (sym->stype== S_STRLIT) { sym=sym->next; continue; }

      // If this is a char pointer or an array of char pointers,
      // replace any values in the initlist (which are symbol ids)
      // with the associated string literal labels.
      // Yes, P_CHAR+2 means array of char pointers.
      if (sym->initlist!=NULL &&
        (sym->type== pointer_to(P_CHAR) || sym->type == P_CHAR+2)) {
        for (i=0; i<sym->nelems; i++)
          if (sym->initlist[i]!=0) {
            litsym= findSymbol(NULL, 0, sym->initlist[i]);
            sym->initlist[i]= litsym->st_label;
          } 
      }
      genglobsym(sym);
      sym=sym->next;
  }
  freeSymtable();		// Clear the symbol table
}
 

// Open the symbol table file and AST file
// Loop:
//  Read in the next AST tree
//  Generate the assembly code
//  Free the in-memory symbol tables
int main(int argc, char **argv) {
  struct ASTnode *node;

  if (argc !=4) {
    fprintf(stderr, "Usage: %s symfile astfile idxfile\n", argv[0]);
    exit(1);
  }

  // Open the symbol table file
  Symfile= fopen(argv[1], "r");
  if (Symfile == NULL) {
    fprintf(stderr, "Can't open %s\n", argv[1]); exit(1);
  }

  // Open the AST file
  Infile= fopen(argv[2], "r");
  if (Infile == NULL) {
    fprintf(stderr, "Can't open %s\n", argv[2]); exit(1);
  }

  // Open the AST index offset file for read/writing
  Idxfile= fopen(argv[3], "w+");
  if (Idxfile == NULL) {
    fprintf(stderr, "Can't open %s\n", argv[3]); exit(1);
  }

  // We write assembly to stdout
  Outfile=stdout;

  mkASTidxfile();		// Build the AST index offset file
  freeSymtable();		// Clear the symbol table
  genpreamble();		// Output the preamble
  allocateGlobals();		// Allocate global variables

  while (1) {
    // Read the next function's top node in from file
    node= loadASTnode(0, 1);
    if (node==NULL) break;

    // Generate the assembly code for the tree
    genAST(node, NOLABEL, NOLABEL, NOLABEL, 0);

    // Free the symbols in the in-memory symbol tables.
    // Also free the AST node we loaded in
    freeSymtable();
    freeASTnode(node);
  }

  genpostamble();               // Output the postamble
  freeSymtable();
  fclose(Infile);
  fclose(Symfile);
  exit(0);
  return(0);
}
