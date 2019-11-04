#ifndef extern_
#define extern_ extern
#endif

// Global variables
// Copyright (c) 2019 Warren Toomey, GPL3

extern_ int Line;		// Current line number
extern_ int Putback;		// Character put back by scanner
extern_ int Functionid;		// Symbol id of the current function
extern_ int Globs;		// Position of next free global symbol slot
extern_ int Locls;		// Position of next free local symbol slot
extern_ FILE *Infile;		// Input and output files
extern_ FILE *Outfile;
extern_ struct token Token;	// Last token scanned
extern_ char Text[TEXTLEN + 1];	// Last identifier scanned
extern_ struct symtable Symtable[NSYMBOLS];	// Global symbol table

extern_ int O_dumpAST;
