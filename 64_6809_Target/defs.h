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
#define ASCMD "as6809 -o "
#define LDCMD "ld6809 -o %s /tmp/crt0.o %s /opt/fcc/lib/6809/libc.a /opt/fcc/lib/6809/lib6809.a -m %s.map"
#define CPPCMD "cpp -nostdinc -isystem "

// Token types
enum {
  T_EOF,

  // Binary operators
  T_ASSIGN, T_ASPLUS, T_ASMINUS,                // 1
  T_ASSTAR, T_ASSLASH, T_ASMOD,                 // 4
  T_QUESTION, T_LOGOR, T_LOGAND,                // 7
  T_OR, T_XOR, T_AMPER,                         // 10
  T_EQ, T_NE,                                   // 13
  T_LT, T_GT, T_LE, T_GE,                       // 15
  T_LSHIFT, T_RSHIFT,                           // 19
  T_PLUS, T_MINUS, T_STAR, T_SLASH, T_MOD,      // 21

  // Other operators
  T_INC, T_DEC, T_INVERT, T_LOGNOT,             // 26

  // Type keywords
  T_VOID, T_CHAR, T_INT, T_LONG,                // 30

  // Other keywords
  T_IF, T_ELSE, T_WHILE, T_FOR, T_RETURN,       // 34
  T_STRUCT, T_UNION, T_ENUM, T_TYPEDEF,         // 39
  T_EXTERN, T_BREAK, T_CONTINUE, T_SWITCH,      // 43
  T_CASE, T_DEFAULT, T_SIZEOF, T_STATIC,        // 47

  // Structural tokens
  T_INTLIT, T_STRLIT, T_SEMI, T_IDENT,          // 51
  T_LBRACE, T_RBRACE, T_LPAREN, T_RPAREN,       // 55
  T_LBRACKET, T_RBRACKET, T_COMMA, T_DOT,       // 59
  T_ARROW, T_COLON, T_ELLIPSIS, T_CHARLIT,      // 63

  // Misc
  T_FILENAME, T_LINENUM				// 67
};

// Token structure
struct token {
  int token;			// Token type, from the enum list above
  char *tokstr;			// String version of the token
  int intvalue;			// For T_INTLIT, the integer value
};

// AST node types. The first few line up
// with the related tokens
enum {
  A_ASSIGN = 1, A_ASPLUS, A_ASMINUS, A_ASSTAR,			//  1
  A_ASSLASH, A_ASMOD, A_TERNARY, A_LOGOR,			//  5
  A_LOGAND, A_OR, A_XOR, A_AND, A_EQ, A_NE, A_LT,		//  9
  A_GT, A_LE, A_GE, A_LSHIFT, A_RSHIFT,				// 16
  A_ADD, A_SUBTRACT, A_MULTIPLY, A_DIVIDE, A_MOD,		// 21
  A_INTLIT, A_STRLIT, A_IDENT, A_GLUE,				// 26
  A_IF, A_WHILE, A_FUNCTION, A_WIDEN, A_RETURN,			// 30
  A_FUNCCALL, A_DEREF, A_ADDR, A_SCALE,				// 35
  A_PREINC, A_PREDEC, A_POSTINC, A_POSTDEC,			// 39
  A_NEGATE, A_INVERT, A_LOGNOT, A_TOBOOL, A_BREAK,		// 43
  A_CONTINUE, A_SWITCH, A_CASE, A_DEFAULT, A_CAST		// 48
};

// Primitive types. The bottom 4 bits is an integer
// value that represents the level of indirection,
// e.g. 0= no pointer, 1= pointer, 2= pointer pointer etc.
enum {
  P_NONE, P_VOID = 16, P_CHAR = 32, P_INT = 48, P_LONG = 64,
  P_STRUCT=80, P_UNION=96
};

// A symbol in the symbol table is
// one of these structural types.
enum {
  S_VARIABLE, S_FUNCTION, S_ARRAY, S_ENUMVAL, S_STRLIT,
  S_STRUCT, S_UNION, S_ENUMTYPE, S_TYPEDEF, S_NOTATYPE
};

// Visibilty class for symbols
enum {
  V_GLOBAL,			// Globally visible symbol
  V_EXTERN,			// External globally visible symbol
  V_STATIC,			// Static symbol, visible in one file
  V_LOCAL,			// Locally visible symbol
  V_PARAM,			// Locally visible function parameter
  V_MEMBER			// Member of a struct or union
};

// Symbol table structure
struct symtable {
  char *name;			// Name of a symbol
  int id;			// Numeric id of the symbol
  int type;			// Primitive type for the symbol
  struct symtable *ctype;	// If struct/union, ptr to that type
  int ctypeid;			// Numeric id of the struct/union type
  int stype;			// Structural type for the symbol
  int class;			// Visibility class for the symbol
  int size;			// Total size in bytes of this symbol
                                // For functions: size 1 means ... (ellipsis)
#define has_ellipsis size
  int nelems;			// Functions: # params. Arrays: # elements.
  int st_hasaddr;		// For locals, 1 if any A_ADDR operation
#define st_endlabel st_posn	// For functions, the end label
#define st_label st_posn	// For string literals, the associated label
  int st_posn;			// For locals, the negative offset
    				// from the stack base pointer.
				// For struct members, the offset of
				// the member from the base of the struct
  int *initlist;		// List of initial values
  struct symtable *next;	// Next symbol in the symbol table
  struct symtable *member;	// List of member of struct, union or enum.
};				// For functions, list of parameters & locals.

// Abstract Syntax Tree structure
struct ASTnode {
  int op;			// "Operation" to be performed on this tree
  int type;			// Type of any expression this tree generates
  struct symtable *ctype;	// If struct/union, ptr to that type
  int rvalue;			// True if the node is an rvalue
  struct ASTnode *left;		// Left, middle and right child trees
  struct ASTnode *mid;
  struct ASTnode *right;
  int nodeid;                   // Node id when tree is serialised
  int leftid;                   // Numeric ids when serialised
  int midid;
  int rightid;
  struct symtable *sym;		// For many AST nodes, the pointer to
  				// the symbol in the symbol table
  char *name;			// The symbol's name (used by serialiser)
  int symid;			// Symbol's unique id (used by serialiser)
#define a_intvalue a_size	// For A_INTLIT, the integer value
  int a_size;			// For A_SCALE, the size to scale by
  int linenum;			// Line number from where this node comes
};

enum {
  NOREG = -1,			// Use NOREG when the AST generation
  				// functions have no register to return
  NOLABEL = 0			// Use NOLABEL when we have no label to
    				// pass to genAST()
};
