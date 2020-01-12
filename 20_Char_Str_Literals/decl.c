#include "defs.h"
#include "data.h"
#include "decl.h"

// Parsing of declarations
// Copyright (c) 2019 Warren Toomey, GPL3


// Parse the current token and return
// a primitive type enum value. Also
// scan in the next token
int parse_type(void) {
  int type;
  switch (Token.token) {
    case T_VOID:
      type = P_VOID;
      break;
    case T_CHAR:
      type = P_CHAR;
      break;
    case T_INT:
      type = P_INT;
      break;
    case T_LONG:
      type = P_LONG;
      break;
    default:
      fatald("Illegal type, token", Token.token);
  }

  // Scan in one or more further '*' tokens 
  // and determine the correct pointer type
  while (1) {
    scan(&Token);
    if (Token.token != T_STAR)
      break;
    type = pointer_to(type);
  }

  // We leave with the next token already scanned
  return (type);
}

// variable_declaration: type identifier ';'
//        | type identifier '[' INTLIT ']' ';'
//        ;
//
// Parse the declaration of a scalar variable or an array
// with a given size.
// The identifier has been scanned & we have the type
void var_declaration(int type) {
  int id;

  // Text now has the identifier's name.
  // If the next token is a '['
  if (Token.token == T_LBRACKET) {
    // Skip past the '['
    scan(&Token);

    // Check we have an array size
    if (Token.token == T_INTLIT) {
      // Add this as a known array and generate its space in assembly.
      // We treat the array as a pointer to its elements' type
      id = addglob(Text, pointer_to(type), S_ARRAY, 0, Token.intvalue);
      genglobsym(id);
    }

    // Ensure we have a following ']'
    scan(&Token);
    match(T_RBRACKET, "]");
  } else {
    // Add this as a known scalar
    // and generate its space in assembly
    id = addglob(Text, type, S_VARIABLE, 0, 1);
    genglobsym(id);
  }

  // Get the trailing semicolon
  semi();
}

//
// function_declaration: type identifier '(' ')' compound_statement   ;
//
// Parse the declaration of a simplistic function.
// The identifier has been scanned & we have the type
struct ASTnode *function_declaration(int type) {
  struct ASTnode *tree, *finalstmt;
  int nameslot, endlabel;

  // Text now has the identifier's name.
  // Get a label-id for the end label, add the function
  // to the symbol table, and set the Functionid global
  // to the function's symbol-id
  endlabel = genlabel();
  nameslot = addglob(Text, type, S_FUNCTION, endlabel, 0);
  Functionid = nameslot;

  // Scan in the parentheses
  lparen();
  rparen();

  // Get the AST tree for the compound statement
  tree = compound_statement();

  // If the function type isn't P_VOID ..
  if (type != P_VOID) {

    // Error if no statements in the function
    if (tree == NULL)
      fatal("No statements in function with non-void type");

    // Check that the last AST operation in the
    // compound statement was a return statement
    finalstmt = (tree->op == A_GLUE) ? tree->right : tree;
    if (finalstmt == NULL || finalstmt->op != A_RETURN)
      fatal("No return for function with non-void type");
  }
  // Return an A_FUNCTION node which has the function's nameslot
  // and the compound statement sub-tree
  return (mkastunary(A_FUNCTION, type, tree, nameslot));
}


// Parse one or more global declarations, either
// variables or functions
void global_declarations(void) {
  struct ASTnode *tree;
  int type;

  while (1) {

    // We have to read past the type and identifier
    // to see either a '(' for a function declaration
    // or a ',' or ';' for a variable declaration.
    // Text is filled in by the ident() call.
    type = parse_type();
    ident();
    if (Token.token == T_LPAREN) {

       // Parse the function declaration and
       // generate the assembly code for it
       tree = function_declaration(type);
       if (O_dumpAST) { dumpAST(tree, NOLABEL, 0); fprintf(stdout, "\n\n"); }
       genAST(tree, NOLABEL, 0);
    } else {

       // Parse the global variable declaration
       var_declaration(type);
    }

    // Stop when we have reached EOF
    if (Token.token == T_EOF)
      break;
  }
}
