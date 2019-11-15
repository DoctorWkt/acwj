#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Structure and enum definitions
// Copyright (c) 2019 Warren Toomey, GPL3

enum {
  TEXTLEN = 512			// Length of identifiers in input
};

// Commands and default filenames
#define AOUT "a.out"
#define ASCMD "as -o "
#define LDCMD "cc -o "

// Token types
enum {
  T_EOF,

  // Binary operators
  T_ASSIGN, T_LOGOR, T_LOGAND,
  T_OR, T_XOR, T_AMPER,
  T_EQ, T_NE,
  T_LT, T_GT, T_LE, T_GE,
  T_LSHIFT, T_RSHIFT,
  T_PLUS, T_MINUS, T_STAR, T_SLASH,

  // Other operators
  T_INC, T_DEC, T_INVERT, T_LOGNOT,

  // Type keywords
  T_VOID, T_CHAR, T_INT, T_LONG,

  // Other keywords
  T_IF, T_ELSE, T_WHILE, T_FOR, T_RETURN, T_STRUCT,

  // Structural tokens
  T_INTLIT, T_STRLIT, T_SEMI, T_IDENT,
  T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN,
  T_LBRACKET, T_RBRACKET, T_COMMA, T_DOT,
  T_ARROW
};

// Token structure
struct token {
  int token;			// Token type, from the enum list above
  int intvalue;			// For T_INTLIT, the integer value
};

// AST node types. The first few line up
// with the related tokens
enum {
  A_ASSIGN = 1, A_LOGOR, A_LOGAND, A_OR, A_XOR, A_AND,
  A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE, A_LSHIFT, A_RSHIFT,
  A_ADD, A_SUBTRACT, A_MULTIPLY, A_DIVIDE,
  A_INTLIT, A_STRLIT, A_IDENT, A_GLUE,
  A_IF, A_WHILE, A_FUNCTION, A_WIDEN, A_RETURN,
  A_FUNCCALL, A_DEREF, A_ADDR, A_SCALE,
  A_PREINC, A_PREDEC, A_POSTINC, A_POSTDEC,
  A_NEGATE, A_INVERT, A_LOGNOT, A_TOBOOL
};

// Primitive types. The bottom 4 bits is an integer
// value that represents the level of indirection,
// e.g. 0= no pointer, 1= pointer, 2= pointer pointer etc.
enum {
  P_NONE, P_VOID = 16, P_CHAR = 32, P_INT = 48, P_LONG = 64,
  P_STRUCT=80
};

// Structural types
enum {
  S_VARIABLE, S_FUNCTION, S_ARRAY
};

// Storage classes
enum {
  C_GLOBAL = 1,			// Globally visible symbol
  C_LOCAL,			// Locally visible symbol
  C_PARAM,			// Locally visible function parameter
  C_STRUCT,			// A struct
  C_MEMBER			// Member of a struct or union
};

// Symbol table structure
struct symtable {
  char *name;			// Name of a symbol
  int type;			// Primitive type for the symbol
  struct symtable *ctype;	// If struct/union, ptr to that type
  int stype;			// Structural type for the symbol
  int class;			// Storage class for the symbol
  union {
    int size;			// Number of elements in the symbol
    int endlabel;		// For functions, the end label
  };
  union {
    int nelems;			// For functions, # of params
    int posn;			// For locals, the negative offset
    				// from the stack base pointer
  };
  struct symtable *next;	// Next symbol in one list
  struct symtable *member;	// First member of a function, struct,
};				// union or enum

// Abstract Syntax Tree structure
struct ASTnode {
  int op;			// "Operation" to be performed on this tree
  int type;			// Type of any expression this tree generates
  int rvalue;			// True if the node is an rvalue
  struct ASTnode *left;		// Left, middle and right child trees
  struct ASTnode *mid;
  struct ASTnode *right;
  struct symtable *sym;		// For many AST nodes, the pointer to
  union {			// the symbol in the symbol table
    int intvalue;		// For A_INTLIT, the integer value
    int size;			// For A_SCALE, the size to scale by
  };
};

enum {
  NOREG = -1,			// Use NOREG when the AST generation
  				// functions have no register to return
  NOLABEL = 0			// Use NOLABEL when we have no label to
    				// pass to genAST()
};
