#include "defs.h"
#include "data.h"
#include "decl.h"

// Parsing of declarations
// Copyright (c) 2019 Warren Toomey, GPL3

static struct symtable *composite_declaration(int type);
static int typedef_declaration(struct symtable **ctype);
static int type_of_typedef(char *name, struct symtable **ctype);
static void enum_declaration(void);

// Parse the current token and return a primitive type enum value,
// a pointer to any composite type and possibly modify
// the class of the type.
static int parse_type(struct symtable **ctype, int *class) {
  int type, exstatic = 1;

  // See if the class has been changed to extern (later, static)
  while (exstatic) {
    switch (Token.token) {
      case T_EXTERN:
	*class = C_EXTERN;
	scan(&Token);
	break;
      default:
	exstatic = 0;
    }
  }

  // Now work on the actual type keyword
  switch (Token.token) {
    case T_VOID:
      type = P_VOID;
      scan(&Token);
      break;
    case T_CHAR:
      type = P_CHAR;
      scan(&Token);
      break;
    case T_INT:
      type = P_INT;
      scan(&Token);
      break;
    case T_LONG:
      type = P_LONG;
      scan(&Token);
      break;

      // For the following, if we have a ';' after the
      // parsing then there is no type, so return -1.
      // Example: struct x {int y; int z};
    case T_STRUCT:
      type = P_STRUCT;
      *ctype = composite_declaration(P_STRUCT);
      if (Token.token == T_SEMI)
	type = -1;
      break;
    case T_UNION:
      type = P_UNION;
      *ctype = composite_declaration(P_UNION);
      if (Token.token == T_SEMI)
	type = -1;
      break;
    case T_ENUM:
      type = P_INT;		// Enums are really ints
      enum_declaration();
      if (Token.token == T_SEMI)
	type = -1;
      break;
    case T_TYPEDEF:
      type = typedef_declaration(ctype);
      if (Token.token == T_SEMI)
	type = -1;
      break;
    case T_IDENT:
      type = type_of_typedef(Text, ctype);
      break;
    default:
      fatals("Illegal type, token", Token.tokstr);
  }
  return (type);
}

// Given a type parsed by parse_type(), scan in any following
// '*' tokens and return the new type
static int parse_stars(int type) {

  while (1) {
    if (Token.token != T_STAR)
      break;
    type = pointer_to(type);
    scan(&Token);
  }
  return (type);
}

static struct symtable *scalar_declaration(char *varname, int type,
					   struct symtable *ctype,
					   int class) {

  // Add this as a known scalar
  switch (class) {
    case C_EXTERN:
    case C_GLOBAL:
      return (addglob(varname, type, ctype, S_VARIABLE, class, 1));
      break;
    case C_LOCAL:
      return (addlocl(varname, type, ctype, S_VARIABLE, 1));
      break;
    case C_PARAM:
      return (addparm(varname, type, ctype, S_VARIABLE, 1));
      break;
    case C_MEMBER:
      return (addmemb(varname, type, ctype, S_VARIABLE, 1));
      break;
  }
  return (NULL);		// Keep -Wall happy
}

static struct symtable *array_declaration(char *varname, int type,
					  struct symtable *ctype, int class) {
  struct symtable *sym;
  // Skip past the '['
  scan(&Token);

  // Check we have an array size
  if (Token.token == T_INTLIT) {
    // Add this as a known array
    // We treat the array as a pointer to its elements' type
    switch (class) {
      case C_EXTERN:
      case C_GLOBAL:
	sym =
	  addglob(varname, pointer_to(type), ctype, S_ARRAY, class,
		  Token.intvalue);
	break;
      case C_LOCAL:
      case C_PARAM:
      case C_MEMBER:
	fatal("For now, declaration of non-global arrays is not implemented");
    }
  }
  // Ensure we have a following ']'
  scan(&Token);
  match(T_RBRACKET, "]");
  return (sym);
}


static int param_declaration_list(struct symtable *oldfuncsym,
				  struct symtable *newfuncsym) {
  int type, paramcnt = 0;
  struct symtable *ctype;
  struct symtable *protoptr = NULL;

  // Get the pointer to the first prototype parameter
  if (oldfuncsym != NULL)
    protoptr = oldfuncsym->member;

  // Loop getting any parameters
  while (Token.token != T_RPAREN) {
    // Get the type of the next parameter
    type = declaration_list(&ctype, C_PARAM, T_COMMA, T_RPAREN);
    if (type == -1)
      fatal("Bad type in parameter list");

    // Ensure the type of this parameter matches the prototype
    if (protoptr != NULL) {
      if (type != protoptr->type)
	fatald("Type doesn't match prototype for parameter", paramcnt + 1);
      protoptr = protoptr->next;
    }
    paramcnt++;

    // Stop when we hit the right parenthesis
    if (Token.token == T_RPAREN)
      break;
    // We need a comma as separator
    comma();
  }

  if (oldfuncsym != NULL && paramcnt != oldfuncsym->nelems)
    fatals("Parameter count mismatch for function", oldfuncsym->name);

  // Return the count of parameters
  return (paramcnt);
}


//
// function_declaration: type identifier '(' parameter_list ')' ;
//      | type identifier '(' parameter_list ')' compound_statement   ;
//
// Parse the declaration of function.
static struct symtable *function_declaration(char *funcname, int type,
					     struct symtable *ctype,
					     int class) {
  struct ASTnode *tree, *finalstmt;
  struct symtable *oldfuncsym, *newfuncsym = NULL;
  int endlabel, paramcnt;

  // Text has the identifier's name. If this exists and is a
  // function, get the id. Otherwise, set oldfuncsym to NULL.
  if ((oldfuncsym = findsymbol(funcname)) != NULL)
    if (oldfuncsym->stype != S_FUNCTION)
      oldfuncsym = NULL;

  // If this is a new function declaration, get a
  // label-id for the end label, and add the function
  // to the symbol table,
  if (oldfuncsym == NULL) {
    endlabel = genlabel();
    // Assumtion: functions only return scalar types, so NULL below
    newfuncsym =
      addglob(funcname, type, NULL, S_FUNCTION, C_GLOBAL, endlabel);
  }
  // Scan in the '(', any parameters and the ')'.
  // Pass in any existing function prototype pointer
  lparen();
  paramcnt = param_declaration_list(oldfuncsym, newfuncsym);
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
  if (Token.token == T_SEMI)
    return (oldfuncsym);

  // This is not just a prototype.
  // Set the Functionid global to the function's symbol pointer
  Functionid = oldfuncsym;

  // Get the AST tree for the compound statement and mark
  // that we have parsed no loops or switches yet
  Looplevel = 0;
  Switchlevel = 0;
  lbrace();
  tree = compound_statement(0);
  rbrace();

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
  // Build the A_FUNCTION node which has the function's symbol pointer
  // and the compound statement sub-tree
  tree = mkastunary(A_FUNCTION, type, tree, oldfuncsym, endlabel);

  // Generate the assembly code for it
  if (O_dumpAST) {
    dumpAST(tree, NOLABEL, 0);
    fprintf(stdout, "\n\n");
  }
  genAST(tree, NOLABEL, NOLABEL, NOLABEL, 0);

  // Now free the symbols associated
  // with this function
  freeloclsyms();
  return (oldfuncsym);
}

// Parse composite type declarations: structs or unions.
// Either find an existing struct/union declaration, or build
// a struct/union symbol table entry and return its pointer.
static struct symtable *composite_declaration(int type) {
  struct symtable *ctype = NULL;
  struct symtable *m;
  int offset;
  int t;

  // Skip the struct/union keyword
  scan(&Token);

  // See if there is a following struct/union name
  if (Token.token == T_IDENT) {
    // Find any matching composite type
    if (type == P_STRUCT)
      ctype = findstruct(Text);
    else
      ctype = findunion(Text);
    scan(&Token);
  }
  // If the next token isn't an LBRACE , this is
  // the usage of an existing struct/union type.
  // Return the pointer to the type.
  if (Token.token != T_LBRACE) {
    if (ctype == NULL)
      fatals("unknown struct/union type", Text);
    return (ctype);
  }
  // Ensure this struct/union type hasn't been
  // previously defined
  if (ctype)
    fatals("previously defined struct/union", Text);

  // Build the composite type and skip the left brace
  if (type == P_STRUCT)
    ctype = addstruct(Text, P_STRUCT, NULL, 0, 0);
  else
    ctype = addunion(Text, P_UNION, NULL, 0, 0);
  scan(&Token);

  // Scan in the list of members
  while (1) {
    // Get the next member. m is used as a dummy
    t= declaration_list(&m, C_MEMBER, T_SEMI, T_RBRACE);
    if (t== -1)
      fatal("Bad type in member list");
    if (Token.token == T_SEMI)
      scan(&Token);
    if (Token.token == T_RBRACE)
      break;
  }

  // Attach to the struct type's node
  rbrace();
  if (Membhead==NULL)
    fatals("No members in struct", ctype->name);
  ctype->member = Membhead;
  Membhead = Membtail = NULL;

  // Set the offset of the initial member
  // and find the first free byte after it
  m = ctype->member;
  m->posn = 0;
  offset = typesize(m->type, m->ctype);

  // Set the position of each successive member in the composite type
  // Unions are easy. For structs, align the member and find the next free byte
  for (m = m->next; m != NULL; m = m->next) {
    // Set the offset for this member
    if (type == P_STRUCT)
      m->posn = genalign(m->type, offset, 1);
    else
      m->posn = 0;

    // Get the offset of the next free byte after this member
    offset += typesize(m->type, m->ctype);
  }

  // Set the overall size of the composite type
  ctype->size = offset;
  return (ctype);
}

// Parse an enum declaration
static void enum_declaration(void) {
  struct symtable *etype = NULL;
  char *name;
  int intval = 0;

  // Skip the enum keyword.
  scan(&Token);

  // If there's a following enum type name, get a
  // pointer to any existing enum type node.
  if (Token.token == T_IDENT) {
    etype = findenumtype(Text);
    name = strdup(Text);	// As it gets tromped soon
    scan(&Token);
  }
  // If the next token isn't a LBRACE, check
  // that we have an enum type name, then return
  if (Token.token != T_LBRACE) {
    if (etype == NULL)
      fatals("undeclared enum type:", name);
    return;
  }
  // We do have an LBRACE. Skip it
  scan(&Token);

  // If we have an enum type name, ensure that it
  // hasn't been declared before.
  if (etype != NULL)
    fatals("enum type redeclared:", etype->name);
  else
    // Build an enum type node for this identifier
    etype = addenum(name, C_ENUMTYPE, 0);

  // Loop to get all the enum values
  while (1) {
    // Ensure we have an identifier
    // Copy it in case there's an int literal coming up
    ident();
    name = strdup(Text);

    // Ensure this enum value hasn't been declared before
    etype = findenumval(name);
    if (etype != NULL)
      fatals("enum value redeclared:", Text);

    // If the next token is an '=', skip it and
    // get the following int literal
    if (Token.token == T_ASSIGN) {
      scan(&Token);
      if (Token.token != T_INTLIT)
	fatal("Expected int literal after '='");
      intval = Token.intvalue;
      scan(&Token);
    }
    // Build an enum value node for this identifier.
    // Increment the value for the next enum identifier.
    etype = addenum(name, C_ENUMVAL, intval++);

    // Bail out on a right curly bracket, else get a comma
    if (Token.token == T_RBRACE)
      break;
    comma();
  }
  scan(&Token);			// Skip over the right curly bracket
}

// Parse a typedef declaration and return the type
// and ctype that it represents
static int typedef_declaration(struct symtable **ctype) {
  int type, class = 0;

  // Skip the typedef keyword.
  scan(&Token);

  // Get the actual type following the keyword
  type = parse_type(ctype, &class);
  if (class != 0)
    fatal("Can't have extern in a typedef declaration");

  // See if the typedef identifier already exists
  if (findtypedef(Text) != NULL)
    fatals("redefinition of typedef", Text);

  // Get any following '*' tokens
  type = parse_stars(type);

  // It doesn't exist so add it to the typedef list
  addtypedef(Text, type, *ctype, 0, 0);
  scan(&Token);
  return (type);
}

// Given a typedef name, return the type it represents
static int type_of_typedef(char *name, struct symtable **ctype) {
  struct symtable *t;

  // Look up the typedef in the list
  t = findtypedef(name);
  if (t == NULL)
    fatals("unknown type", name);
  scan(&Token);
  *ctype = t->ctype;
  return (t->type);
}

static void array_initialisation(struct symtable *sym, int type,
				 struct symtable *ctype, int class) {
  fatal("No array initialisation yet!");
}

// Parse the declaration of a variable or function.
// The type and any following '*'s have been scanned, and we
// have the identifier in the Token variable.
// The class argument is the variable's class.
// Return a pointer to the symbol's entry in the symbol table
static struct symtable *symbol_declaration(int type, struct symtable *ctype,
					   int class) {
  struct symtable *sym = NULL;
  char *varname = strdup(Text);
  int stype = S_VARIABLE;
  // struct ASTnode *expr = NULL;

  // Assume it will be a scalar variable.
  // Ensure that we have an identifier. 
  // We copied it above so we can scan more tokens in, e.g.
  // an assignment expression for a local variable.
  ident();

  // Deal with function declarations
  if (Token.token == T_LPAREN) {
    return (function_declaration(varname, type, ctype, class));
  }
  // See if this array or scalar variable has already been declared
  switch (class) {
    case C_EXTERN:
    case C_GLOBAL:
      if (findglob(varname) != NULL)
	fatals("Duplicate global variable declaration", varname);
    case C_LOCAL:
    case C_PARAM:
      if (findlocl(varname) != NULL)
	fatals("Duplicate local variable declaration", varname);
    case C_MEMBER:
      if (findmember(varname) != NULL)
	fatals("Duplicate struct/union member declaration", varname);
  }

  // Add the array or scalar variable to the symbol table
  if (Token.token == T_LBRACKET) {
    sym = array_declaration(varname, type, ctype, class);
    stype = S_ARRAY;
  } else
    sym = scalar_declaration(varname, type, ctype, class);

  // The array or scalar variable is being initialised
  if (Token.token == T_ASSIGN) {
    // Not possible for a parameter or member
    if (class == C_PARAM)
      fatals("Initialisation of a parameter not permitted", varname);
    if (class == C_MEMBER)
      fatals("Initialisation of a member not permitted", varname);
    scan(&Token);

    // Array initialisation
    if (stype == S_ARRAY)
      array_initialisation(sym, type, ctype, class);
    else {
      fatal("Scalar variable initialisation not done yet");
      // Variable initialisation
      // if (class== C_LOCAL)
      // Local variable, parse the expression
      // expr= binexpr(0);
      // else write more code!
    }
  }
  // Generate the storage for the array or scalar variable. SOON.
  // genstorage(sym, expr);
  return (sym);
}

// Parse a list of symbols where there is an initial type.
// Return the type of the symbols. et1 and et2 are end tokens.
int declaration_list(struct symtable **ctype, int class, int et1, int et2) {
  int inittype, type;
  struct symtable *sym;

  // Get the initial type. If -1, it was
  // a composite type definition, return this
  if ((inittype = parse_type(ctype, &class)) == -1)
    return (inittype);

  // Now parse the list of symbols
  while (1) {
    // See if this symbol is a pointer
    type = parse_stars(inittype);

    // Parse this symbol
    sym = symbol_declaration(type, *ctype, class);

    // We parsed a function, there is no list so leave
    if (sym->stype == S_FUNCTION) {
      if (class != C_GLOBAL)
        fatal("Function definition not at global level");
      return (type);
    }

    // We are at the end of the list, leave
    if (Token.token == et1 || Token.token == et2)
      return (type);

    // Otherwise, we need a comma as separator
    comma();
  }
}

// Parse one or more global declarations, either
// variables, functions or structs
void global_declarations(void) {
  struct symtable *ctype;
  while (Token.token != T_EOF) {
    declaration_list(&ctype, C_GLOBAL, T_SEMI, T_EOF);
    // Skip any semicolons and right curly brackets
    if (Token.token == T_SEMI)
      scan(&Token);
  }
}
