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
extern_ char *Infilename;		// Name of file we are parsing
extern_ char *Outfilename;		// Name of file we opened as Outfile
extern_ struct token Token;		// Last token scanned
extern_ char Text[TEXTLEN + 1];		// Last identifier scanned

// Symbol table lists
extern_ struct symtable *Globhead, *Globtail;	  // Global variables and functions
extern_ struct symtable *Loclhead, *Locltail;	  // Local variables
extern_ struct symtable *Parmhead, *Parmtail;	  // Local parameters
extern_ struct symtable *Membhead, *Membtail;	  // Temp list of struct/union members
extern_ struct symtable *Structhead, *Structtail; // List of struct types
extern_ struct symtable *Unionhead, *Uniontail;   // List of union types
extern_ struct symtable *Enumhead,  *Enumtail;    // List of enum types and values
extern_ struct symtable *Typehead,  *Typetail;    // List of typedefs

// Command-line flags
extern_ int O_dumpAST;		// If true, dump the AST trees
extern_ int O_keepasm;		// If true, keep any assembly files
extern_ int O_assemble;		// If true, assemble the assembly files
extern_ int O_dolink;		// If true, link the object files
extern_ int O_verbose;		// If true, print info on compilation stages
