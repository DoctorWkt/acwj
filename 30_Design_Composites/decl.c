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
// The identifier has been scanned & we have the type.
// class is the variable's class
// Return the pointer to variable's entry in the symbol table
struct symtable *var_declaration(int type, int class) {
  struct symtable *sym = NULL;

  // See if this has already been declared
  switch (class) {
    case C_GLOBAL:
      if (findglob(Text) != NULL)
	fatals("Duplicate global variable declaration", Text);
    case C_LOCAL:
    case C_PARAM:
      if (findlocl(Text) != NULL)
	fatals("Duplicate local variable declaration", Text);
  }

  // Text now has the identifier's name.
  // If the next token is a '['
  if (Token.token == T_LBRACKET) {
    // Skip past the '['
    scan(&Token);

    // Check we have an array size
    if (Token.token == T_INTLIT) {
      // Add this as a known array and generate its space in assembly.
      // We treat the array as a pointer to its elements' type
      switch (class) {
	case C_GLOBAL:
	  sym =
	    addglob(Text, pointer_to(type), S_ARRAY, class, Token.intvalue);
	  break;
	case C_LOCAL:
	case C_PARAM:
	  fatal("For now, declaration of local arrays is not implemented");
      }
    }
    // Ensure we have a following ']'
    scan(&Token);
    match(T_RBRACKET, "]");
  } else {
    // Add this as a known scalar
    // and generate its space in assembly
    switch (class) {
      case C_GLOBAL:
	sym = addglob(Text, type, S_VARIABLE, class, 1);
	break;
      case C_LOCAL:
	sym = addlocl(Text, type, S_VARIABLE, class, 1);
	break;
      case C_PARAM:
	sym = addparm(Text, type, S_VARIABLE, class, 1);
	break;
    }
  }
  return (sym);
}

// param_declaration: <null>
//           | variable_declaration
//           | variable_declaration ',' param_declaration
//
// Parse the parameters in parentheses after the function name.
// Add them as symbols to the symbol table and return the number
// of parameters. If funcsym is not NULL, there is an existing function
// prototype, and the function has this symbol table pointer.
static int param_declaration(struct symtable *funcsym) {
  int type;
  int paramcnt = 0;
  struct symtable *protoptr = NULL;

  // If there is a prototype, get the pointer
  // to the first prototype parameter
  if (funcsym != NULL)
    protoptr = funcsym->member;

  // Loop until the final right parentheses
  while (Token.token != T_RPAREN) {
    // Get the type and identifier
    // and add it to the symbol table
    type = parse_type();
    ident();

    // We have an existing prototype.
    // Check that this type matches the prototype.
    if (protoptr != NULL) {
      if (type != protoptr->type)
	fatald("Type doesn't match prototype for parameter", paramcnt + 1);
      protoptr = protoptr->next;
    } else {
      // Add a new parameter to the new parameter list
      var_declaration(type, C_PARAM);
    }
    paramcnt++;

    // Must have a ',' or ')' at this point
    switch (Token.token) {
      case T_COMMA:
	scan(&Token);
	break;
      case T_RPAREN:
	break;
      default:
	fatald("Unexpected token in parameter list", Token.token);
    }
  }

  // Check that the number of parameters in this list matches
  // any existing prototype
  if ((funcsym != NULL) && (paramcnt != funcsym->nelems))
    fatals("Parameter count mismatch for function", funcsym->name);

  // Return the count of parameters
  return (paramcnt);
}

//
// function_declaration: type identifier '(' parameter_list ')' ;
//      | type identifier '(' parameter_list ')' compound_statement   ;
//
// Parse the declaration of function.
// The identifier has been scanned & we have the type.
struct ASTnode *function_declaration(int type) {
  struct ASTnode *tree, *finalstmt;
  struct symtable *oldfuncsym, *newfuncsym = NULL;
  int endlabel, paramcnt;

  // Text has the identifier's name. If this exists and is a
  // function, get the id. Otherwise, set oldfuncsym to NULL.
  if ((oldfuncsym = findsymbol(Text)) != NULL)
    if (oldfuncsym->stype != S_FUNCTION)
      oldfuncsym = NULL;

  // If this is a new function declaration, get a
  // label-id for the end label, and add the function
  // to the symbol table,
  if (oldfuncsym == NULL) {
    endlabel = genlabel();
    newfuncsym = addglob(Text, type, S_FUNCTION, C_GLOBAL, endlabel);
  }
  // Scan in the '(', any parameters and the ')'.
  // Pass in any existing function prototype pointer
  lparen();
  paramcnt = param_declaration(oldfuncsym);
  rparen();

  // If this is a new function declaration, update the
  // function symbol entry with the number of parameters.
  // Also copy the parameter list into the function's node.
  if (newfuncsym) {
    newfuncsym->nelems = paramcnt;
    newfuncsym->member = Parmhead;
    oldfuncsym = newfuncsym;
  }
  // Clear out the parameter list
  Parmhead = Parmtail = NULL;

  // Declaration ends in a semicolon, only a prototype.
  if (Token.token == T_SEMI) {
    scan(&Token);
    return (NULL);
  }
  // This is not just a prototype.
  // Set the Functionid global to the function's symbol pointer
  Functionid = oldfuncsym;

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
  // Return an A_FUNCTION node which has the function's symbol pointer
  // and the compound statement sub-tree
  return (mkastunary(A_FUNCTION, type, tree, oldfuncsym, endlabel));
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

      // Parse the function declaration
      tree = function_declaration(type);

      // Only a function prototype, no code
      if (tree == NULL)
	continue;

      // A real function, generate the assembly code for it
      if (O_dumpAST) {
	dumpAST(tree, NOLABEL, 0);
	fprintf(stdout, "\n\n");
      }
      genAST(tree, NOLABEL, 0);

      // Now free the symbols associated
      // with this function
      freeloclsyms();
    } else {

      // Parse the global variable declaration
      // and skip past the trailing semicolon
      var_declaration(type, C_GLOBAL);
      semi();
    }

    // Stop when we have reached EOF
    if (Token.token == T_EOF)
      break;
  }
}
