#include "defs.h"
#include "data.h"
#include "decl.h"

// Parsing of expressions
// Copyright (c) 2019 Warren Toomey, GPL3

// expression_list: <null>
//        | expression
//        | expression ',' expression_list
//        ;

// Parse a list of zero or more comma-separated expressions and
// return an AST composed of A_GLUE nodes with the left-hand child
// being the sub-tree of previous expressions (or NULL) and the right-hand
// child being the next expression. Each A_GLUE node will have size field
// set to the number of expressions in the tree at this point. If no
// expressions are parsed, NULL is returned
struct ASTnode *expression_list(int endtoken) {
  struct ASTnode *tree = NULL;
  struct ASTnode *child = NULL;
  int exprcount = 0;

  // Loop until the end token
  while (Token.token != endtoken) {

    // Parse the next expression and increment the expression count
    child = binexpr(0);
    exprcount++;

    // Build an A_GLUE AST node with the previous tree as the left child
    // and the new expression as the right child. Store the expression count.
    tree =
      mkastnode(A_GLUE, P_NONE, NULL, tree, NULL, child, NULL, exprcount);

    // Stop when we reach the end token
    if (Token.token == endtoken)
      break;

    // Must have a ',' at this point
    match(T_COMMA, ",");
  }

  // Return the tree of expressions
  return (tree);
}

// Parse a function call and return its AST
static struct ASTnode *funccall(void) {
  struct ASTnode *tree;
  struct symtable *funcptr;

  // Check that the identifier has been defined as a function,
  // then make a leaf node for it.
  if ((funcptr = findsymbol(Text)) == NULL || funcptr->stype != S_FUNCTION) {
    fatals("Undeclared function", Text);
  }

  // Get the '('
  lparen();

  // Parse the argument expression list
  tree = expression_list(T_RPAREN);

  // XXX Check type of each argument against the function's prototype

  // Build the function call AST node. Store the
  // function's return type as this node's type.
  // Also record the function's symbol-id
  tree =
    mkastunary(A_FUNCCALL, funcptr->type, funcptr->ctype, tree, funcptr, 0);

  // Get the ')'
  rparen();
  return (tree);
}

// Parse the index into an array and return an AST tree for it
static struct ASTnode *array_access(struct ASTnode *left) {
  struct ASTnode *right;

  // Check that the sub-tree is a pointer
  if (!ptrtype(left->type))
    fatal("Not an array or pointer");

  // Get the '['
  scan(&Token);

  // Parse the following expression
  right = binexpr(0);

  // Get the ']'
  match(T_RBRACKET, "]");

  // Ensure that this is of int type
  if (!inttype(right->type))
    fatal("Array index is not of integer type");

  // Make the left tree an rvalue
  left->rvalue = 1;

  // Scale the index by the size of the element's type
  right = modify_type(right, left->type, left->ctype, A_ADD);

  // Return an AST tree where the array's base has the offset added to it,
  // and dereference the element. Still an lvalue at this point.
  left =
    mkastnode(A_ADD, left->type, left->ctype, left, NULL, right, NULL, 0);
  left =
    mkastunary(A_DEREF, value_at(left->type), left->ctype, left, NULL, 0);
  return (left);
}

// Parse the member reference of a struct or union
// and return an AST tree for it. If withpointer is true,
// the access is through a pointer to the member.
static struct ASTnode *member_access(struct ASTnode *left, int withpointer) {
  struct ASTnode *right;
  struct symtable *typeptr;
  struct symtable *m;

  // Check that the left AST tree is a pointer to struct or union
  if (withpointer && left->type != pointer_to(P_STRUCT)
      && left->type != pointer_to(P_UNION))
    fatal("Expression is not a pointer to a struct/union");

  // Or, check that the left AST tree is a struct or union.
  // If so, change it from an A_IDENT to an A_ADDR so that
  // we get the base address, not the value at this address.
  if (!withpointer) {
    if (left->type == P_STRUCT || left->type == P_UNION)
      left->op = A_ADDR;
    else
      fatal("Expression is not a struct/union");
  }

  // Get the details of the composite type
  typeptr = left->ctype;

  // Skip the '.' or '->' token and get the member's name
  scan(&Token);
  ident();

  // Find the matching member's name in the type
  // Die if we can't find it
  for (m = typeptr->member; m != NULL; m = m->next)
    if (!strcmp(m->name, Text))
      break;
  if (m == NULL)
    fatals("No member found in struct/union: ", Text);

  // Make the left tree an rvalue
  left->rvalue = 1;

  // Build an A_INTLIT node with the offset
  right = mkastleaf(A_INTLIT, P_INT, NULL, NULL, m->st_posn);

  // Add the member's offset to the base of the struct/union
  // and dereference it. Still an lvalue at this point
  left =
    mkastnode(A_ADD, pointer_to(m->type), m->ctype, left, NULL, right, NULL,
	      0);
  left = mkastunary(A_DEREF, m->type, m->ctype, left, NULL, 0);
  return (left);
}

// Parse a parenthesised expression and
// return an AST node representing it.
static struct ASTnode *paren_expression(int ptp) {
  struct ASTnode *n;
  int type = 0;
  struct symtable *ctype = NULL;

  // Beginning of a parenthesised expression, skip the '('.
  scan(&Token);

  // If the token after is a type identifier, this is a cast expression
  switch (Token.token) {
  case T_IDENT:
    // We have to see if the identifier matches a typedef.
    // If not, treat it as an expression.
    if (findtypedef(Text) == NULL) {
      n = binexpr(0);	// ptp is zero as expression inside ( )
      break;
    }
  case T_VOID:
  case T_CHAR:
  case T_INT:
  case T_LONG:
  case T_STRUCT:
  case T_UNION:
  case T_ENUM:
    // Get the type inside the parentheses
    type = parse_cast(&ctype);

    // Skip the closing ')' and then parse the following expression
    rparen();

  default:
    n = binexpr(ptp);		// Scan in the expression. We pass in ptp
				// as the cast doesn't change the
				// expression's precedence
  }

  // We now have at least an expression in n, and possibly a non-zero type
  // in type if there was a cast. Skip the closing ')' if there was no cast.
  if (type == 0)
    rparen();
  else
    // Otherwise, make a unary AST node for the cast
    n = mkastunary(A_CAST, type, ctype, n, NULL, 0);
  return (n);
}

// Parse a primary factor and return an
// AST node representing it.
static struct ASTnode *primary(int ptp) {
  struct ASTnode *n;
  struct symtable *enumptr;
  struct symtable *varptr;
  int id;
  int type = 0;
  int size, class;
  struct symtable *ctype;

  switch (Token.token) {
  case T_STATIC:
  case T_EXTERN:
    fatal("Compiler doesn't support static or extern local declarations");
  case T_SIZEOF:
    // Skip the T_SIZEOF and ensure we have a left parenthesis
    scan(&Token);
    if (Token.token != T_LPAREN)
      fatal("Left parenthesis expected after sizeof");
    scan(&Token);

    // Get the type inside the parentheses
    type = parse_stars(parse_type(&ctype, &class));

    // Get the type's size
    size = typesize(type, ctype);
    rparen();

    // Make a leaf node int literal with the size
    return (mkastleaf(A_INTLIT, P_INT, NULL, NULL, size));

  case T_INTLIT:
    // For an INTLIT token, make a leaf AST node for it.
    // Make it a P_CHAR if it's within the P_CHAR range
    if (Token.intvalue >= 0 && Token.intvalue < 256)
      n = mkastleaf(A_INTLIT, P_CHAR, NULL, NULL, Token.intvalue);
    else
      n = mkastleaf(A_INTLIT, P_INT, NULL, NULL, Token.intvalue);
    break;

  case T_STRLIT:
    // For a STRLIT token, generate the assembly for it.
    id = genglobstr(Text, 0);

    // For successive STRLIT tokens, append their contents
    // to this one
    while (1) {
      scan(&Peektoken);
      if (Peektoken.token != T_STRLIT) break;
      genglobstr(Text, 1);
      scan(&Token);	// To skip it properly
    }

    // Now make a leaf AST node for it. id is the string's label.
    genglobstrend();
    n = mkastleaf(A_STRLIT, pointer_to(P_CHAR), NULL, NULL, id);
    break;

  case T_IDENT:
    // If the identifier matches an enum value,
    // return an A_INTLIT node
    if ((enumptr = findenumval(Text)) != NULL) {
      n = mkastleaf(A_INTLIT, P_INT, NULL, NULL, enumptr->st_posn);
      break;
    }

    // See if this identifier exists as a symbol. For arrays, set rvalue to 1.
    if ((varptr = findsymbol(Text)) == NULL)
      fatals("Unknown variable or function", Text);
    switch (varptr->stype) {
    case S_VARIABLE:
      n = mkastleaf(A_IDENT, varptr->type, varptr->ctype, varptr, 0);
      break;
    case S_ARRAY:
      n = mkastleaf(A_ADDR, varptr->type, varptr->ctype, varptr, 0);
      n->rvalue = 1;
      break;
    case S_FUNCTION:
      // Function call, see if the next token is a left parenthesis
      scan(&Token);
      if (Token.token != T_LPAREN)
	fatals("Function name used without parentheses", Text);
      return (funccall());
    default:
      fatals("Identifier not a scalar or array variable", Text);
    }

    break;

  case T_LPAREN:
    return (paren_expression(ptp));

  default:
    fatals("Expecting a primary expression, got token", Token.tokstr);
  }

  // Scan in the next token and return the leaf node
  scan(&Token);
  return (n);
}

// Parse a postfix expression and return
// an AST node representing it. The
// identifier is already in Text.
static struct ASTnode *postfix(int ptp) {
  struct ASTnode *n;

  // Get the primary expression
  n = primary(ptp);

  // Loop until there are no more postfix operators
  while (1) {
    switch (Token.token) {
    case T_LBRACKET:
      // An array reference
      n = array_access(n);
      break;

    case T_DOT:
      // Access into a struct or union
      n = member_access(n, 0);
      break;

    case T_ARROW:
      // Pointer access into a struct or union
      n = member_access(n, 1);
      break;

    case T_INC:
      // Post-increment: skip over the token
      if (n->rvalue == 1)
	fatal("Cannot ++ on rvalue");
      scan(&Token);

      // Can't do it twice
      if (n->op == A_POSTINC || n->op == A_POSTDEC)
	fatal("Cannot ++ and/or -- more than once");

      // and change the AST operation
      n->op = A_POSTINC;
      break;

    case T_DEC:
      // Post-decrement: skip over the token
      if (n->rvalue == 1)
	fatal("Cannot -- on rvalue");
      scan(&Token);

      // Can't do it twice
      if (n->op == A_POSTINC || n->op == A_POSTDEC)
	fatal("Cannot ++ and/or -- more than once");

      // and change the AST operation
      n->op = A_POSTDEC;
      break;

    default:
      return (n);
    }
  }

  return (NULL);		// Keep -Wall happy
}


// Convert a binary operator token into a binary AST operation.
// We rely on a 1:1 mapping from token to AST operation
static int binastop(int tokentype) {
  if (tokentype > T_EOF && tokentype <= T_MOD)
    return (tokentype);
  fatals("Syntax error, token", Tstring[tokentype]);
  return (0);			// Keep -Wall happy
}

// Return true if a token is right-associative,
// false otherwise.
static int rightassoc(int tokentype) {
  if (tokentype >= T_ASSIGN && tokentype <= T_ASSLASH)
    return (1);
  return (0);
}

// Operator precedence for each token. Must
// match up with the order of tokens in defs.h
static int OpPrec[] = {
  0, 10, 10,			// T_EOF, T_ASSIGN, T_ASPLUS,
  10, 10,			// T_ASMINUS, T_ASSTAR,
  10, 10,			// T_ASSLASH, T_ASMOD,
  15,				// T_QUESTION,
  20, 30,			// T_LOGOR, T_LOGAND
  40, 50, 60,			// T_OR, T_XOR, T_AMPER 
  70, 70,			// T_EQ, T_NE
  80, 80, 80, 80,		// T_LT, T_GT, T_LE, T_GE
  90, 90,			// T_LSHIFT, T_RSHIFT
  100, 100,			// T_PLUS, T_MINUS
  110, 110, 110			// T_STAR, T_SLASH, T_MOD
};

// Check that we have a binary operator and
// return its precedence.
static int op_precedence(int tokentype) {
  int prec;
  if (tokentype > T_MOD)
    fatals("Token with no precedence in op_precedence:", Tstring[tokentype]);
  prec = OpPrec[tokentype];
  if (prec == 0)
    fatals("Syntax error, token", Tstring[tokentype]);
  return (prec);
}

// prefix_expression: postfix_expression
//     | '*'  prefix_expression
//     | '&'  prefix_expression
//     | '-'  prefix_expression
//     | '++' prefix_expression
//     | '--' prefix_expression
//     ;

// Parse a prefix expression and return 
// a sub-tree representing it.
static struct ASTnode *prefix(int ptp) {
  struct ASTnode *tree;
  switch (Token.token) {
  case T_AMPER:
    // Get the next token and parse it
    // recursively as a prefix expression
    scan(&Token);
    tree = prefix(ptp);

    // Ensure that it's an identifier
    if (tree->op != A_IDENT)
      fatal("& operator must be followed by an identifier");

    // Prevent '&' being performed on an array
    if (tree->sym->stype == S_ARRAY)
      fatal("& operator cannot be performed on an array");

    // Now change the operator to A_ADDR and the type to
    // a pointer to the original type
    tree->op = A_ADDR;
    tree->type = pointer_to(tree->type);
    break;
  case T_STAR:
    // Get the next token and parse it
    // recursively as a prefix expression.
    // Make it an rvalue
    scan(&Token);
    tree = prefix(ptp);
    tree->rvalue= 1;

    // Ensure the tree's type is a pointer
    if (!ptrtype(tree->type))
      fatal("* operator must be followed by an expression of pointer type");

    // Prepend an A_DEREF operation to the tree
    tree =
      mkastunary(A_DEREF, value_at(tree->type), tree->ctype, tree, NULL, 0);
    break;
  case T_MINUS:
    // Get the next token and parse it
    // recursively as a prefix expression
    scan(&Token);
    tree = prefix(ptp);

    // Prepend a A_NEGATE operation to the tree and
    // make the child an rvalue. Because chars are unsigned,
    // also widen this if needed to int so that it's signed
    tree->rvalue = 1;
    if (tree->type == P_CHAR)
      tree->type = P_INT;
    tree = mkastunary(A_NEGATE, tree->type, tree->ctype, tree, NULL, 0);
    break;
  case T_INVERT:
    // Get the next token and parse it
    // recursively as a prefix expression
    scan(&Token);
    tree = prefix(ptp);

    // Prepend a A_INVERT operation to the tree and
    // make the child an rvalue.
    tree->rvalue = 1;
    tree = mkastunary(A_INVERT, tree->type, tree->ctype, tree, NULL, 0);
    break;
  case T_LOGNOT:
    // Get the next token and parse it
    // recursively as a prefix expression
    scan(&Token);
    tree = prefix(ptp);

    // Prepend a A_LOGNOT operation to the tree and
    // make the child an rvalue.
    tree->rvalue = 1;
    tree = mkastunary(A_LOGNOT, tree->type, tree->ctype, tree, NULL, 0);
    break;
  case T_INC:
    // Get the next token and parse it
    // recursively as a prefix expression
    scan(&Token);
    tree = prefix(ptp);

    // For now, ensure it's an identifier
    if (tree->op != A_IDENT)
      fatal("++ operator must be followed by an identifier");

    // Prepend an A_PREINC operation to the tree
    tree = mkastunary(A_PREINC, tree->type, tree->ctype, tree, NULL, 0);
    break;
  case T_DEC:
    // Get the next token and parse it
    // recursively as a prefix expression
    scan(&Token);
    tree = prefix(ptp);

    // For now, ensure it's an identifier
    if (tree->op != A_IDENT)
      fatal("-- operator must be followed by an identifier");

    // Prepend an A_PREDEC operation to the tree
    tree = mkastunary(A_PREDEC, tree->type, tree->ctype, tree, NULL, 0);
    break;
  default:
    tree = postfix(ptp);
  }
  return (tree);
}

// Return an AST tree whose root is a binary operator.
// Parameter ptp is the previous token's precedence.
struct ASTnode *binexpr(int ptp) {
  struct ASTnode *left, *right;
  struct ASTnode *ltemp, *rtemp;
  int ASTop;
  int tokentype;

  // Get the tree on the left.
  // Fetch the next token at the same time.
  left = prefix(ptp);

  // If we hit one of several terminating tokens, return just the left node
  tokentype = Token.token;
  if (tokentype == T_SEMI || tokentype == T_RPAREN ||
      tokentype == T_RBRACKET || tokentype == T_COMMA ||
      tokentype == T_COLON || tokentype == T_RBRACE) {
    left->rvalue = 1;
    return (left);
  }

  // While the precedence of this token is more than that of the
  // previous token precedence, or it's right associative and
  // equal to the previous token's precedence
  while ((op_precedence(tokentype) > ptp) ||
	 (rightassoc(tokentype) && op_precedence(tokentype) == ptp)) {
    // Fetch in the next integer literal
    scan(&Token);

    // Recursively call binexpr() with the
    // precedence of our token to build a sub-tree
    right = binexpr(OpPrec[tokentype]);

    // Determine the operation to be performed on the sub-trees
    ASTop = binastop(tokentype);

    switch (ASTop) {
    case A_TERNARY:
      // Ensure we have a ':' token, scan in the expression after it
      match(T_COLON, ":");
      ltemp = binexpr(0);

      // Build and return the AST for this statement. Use the middle
      // expression's type as the return type. XXX We should also
      // consider the third expression's type.
      return (mkastnode
	      (A_TERNARY, right->type, right->ctype, left, right, ltemp,
	       NULL, 0));

    case A_ASSIGN:
      // Assignment
      // Make the right tree into an rvalue
      right->rvalue = 1;

      // Ensure the right's type matches the left
      right = modify_type(right, left->type, left->ctype, 0);
      if (right == NULL)
	fatal("Incompatible expression in assignment");

      // Make an assignment AST tree. However, switch
      // left and right around, so that the right expression's 
      // code will be generated before the left expression
      ltemp = left;
      left = right;
      right = ltemp;
      break;

    default:
      // We are not doing a ternary or assignment, so both trees should
      // be rvalues. Convert both trees into rvalue if they are lvalue trees
      left->rvalue = 1;
      right->rvalue = 1;

      // Ensure the two types are compatible by trying
      // to modify each tree to match the other's type.
      ltemp = modify_type(left, right->type, right->ctype, ASTop);
      rtemp = modify_type(right, left->type, left->ctype, ASTop);
      if (ltemp == NULL && rtemp == NULL)
	fatal("Incompatible types in binary expression");
      if (ltemp != NULL)
	left = ltemp;
      if (rtemp != NULL)
	right = rtemp;
    }

    // Join that sub-tree with ours. Convert the token
    // into an AST operation at the same time.
    left =
      mkastnode(binastop(tokentype), left->type, left->ctype, left, NULL,
		right, NULL, 0);

    // Some operators produce an int result regardless of their operands
    switch (binastop(tokentype)) {
    case A_LOGOR:
    case A_LOGAND:
    case A_EQ:
    case A_NE:
    case A_LT:
    case A_GT:
    case A_LE:
    case A_GE:
      left->type = P_INT;
    }

    // Update the details of the current token.
    // If we hit a terminating token, return just the left node
    tokentype = Token.token;
    if (tokentype == T_SEMI || tokentype == T_RPAREN ||
	tokentype == T_RBRACKET || tokentype == T_COMMA ||
	tokentype == T_COLON || tokentype == T_RBRACE) {
      left->rvalue = 1;
      return (left);
    }
  }

  // Return the tree we have when the precedence
  // is the same or lower
  left->rvalue = 1;
  return (left);
}
