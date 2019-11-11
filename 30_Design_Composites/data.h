#ifndef extern_
#define extern_ extern
#endif

// Global variables
// Copyright (c) 2019 Warren Toomey, GPL3

extern_ int Line;		     	// Current line number
extern_ int Putback;		     	// Character put back by scanner
extern_ struct symtable *Functionid; 	// Symbol ptr of the current function
extern_ FILE *Infile;		     	// Input and output files
extern_ FILE *Outfile;
extern_ char *Outfilename;		// Name of file we opened as Outfile
extern_ struct token Token;		// Last token scanned
extern_ char Text[TEXTLEN + 1];		// Last identifier scanned

// Symbol table lists
struct symtable *Globhead, *Globtail;	// Global variables and functions
struct symtable *Loclhead, *Locltail;	// Local variables
struct symtable *Parmhead, *Parmtail;	// Local parameters
struct symtable *Comphead, *Comptail;	// Composite types

// Command-line flags
extern_ int O_dumpAST;		// If true, dump the AST trees
extern_ int O_keepasm;		// If true, keep any assembly files
extern_ int O_assemble;		// If true, assemble the assembly files
extern_ int O_dolink;		// If true, link the object files
extern_ int O_verbose;		// If true, print info on compilation stages
