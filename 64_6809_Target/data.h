#ifndef extern_
#define extern_ extern
#endif

// Global variables
// Copyright (c) 2019 Warren Toomey, GPL3

extern_ int Line;		     	// Current line number
extern_ int Linestart;		     	// True if at start of a line
extern_ int Putback;		     	// Character put back by scanner
extern_ struct symtable *Functionid; 	// Symbol ptr of the current function
extern_ FILE *Infile;		     	// Input and output files
extern_ FILE *Outfile;
extern_ FILE *Symfile;			// Symbol table file
extern_ FILE *Idxfile;			// AST offset index file
extern_ char *Infilename;		// Name of file we are parsing
extern_ struct token Token;		// Last token scanned
extern_ struct token Peektoken;		// A look-ahead token
extern_ char Text[TEXTLEN + 1];		// Last identifier scanned
extern_ int Looplevel;			// Depth of nested loops
extern_ int Switchlevel;		// Depth of nested switches
extern char *Tstring[];			// List of token strings
