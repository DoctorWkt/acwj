#include "defs.h"
#include "data.h"
#include "decl.h"

// Parsing of expressions
// Copyright (c) 2019 Warren Toomey, GPL3

// Parse a function call with a single expression
// argument and return its AST
static struct ASTnode *funccall(void) {
  struct ASTnode *tree;
  int id;

  // Check that the identifier has been defined as a function,
  // then make a leaf node for it.
  if ((id = findsymbol(Text)) == -1 || Symtable[id].stype != S_FUNCTION) {
    fatals("Undeclared function", Text);
  }
  // Get the '('
  lparen();

  // Parse the following expression
  tree = binexpr(0);

  // Build the function call AST node. Store the
  // function's return type as this node's type.
  // Also record the function's symbol-id
  tree = mkastunary(A_FUNCCALL, Symtable[id].type, tree, id);

  // Get the ')'
  rparen();
  return (tree);
}

// Parse the index into an array and
// return an AST tree for it
static struct ASTnode *array_access(void) {
  struct ASTnode *left, *right;
  int id;

  // Check that the identifier has been defined as an array
  // then make a leaf node for it that points at the base
  if ((id = findsymbol(Text)) == -1 || Symtable[id].stype != S_ARRAY) {
    fatals("Undeclared array", Text);
  }
  left = mkastleaf(A_ADDR, Symtable[id].type, id);

  // Get the '['
  scan(&Token);

  // Parse the following expression
  right = binexpr(0);

  // Get the ']'
  match(T_RBRACKET, "]");

  // Ensure that this is of int type
  if (!inttype(right->type))
    fatal("Array index is not of integer type");

  // Scale the index by the size of the element's type
  right = modify_type(right, left->type, A_ADD);

  // Return an AST tree where the array's base has the offset
  // added to it, and dereference the element. Still an lvalue
  // at this point.
  left = mkastnode(A_ADD, Symtable[id].type, left, NULL, right, 0);
  left = mkastunary(A_DEREF, value_at(left->type), left, 0);
  return (left);
}

// Parse a postfix expression and return
// an AST node representing it. The
// identifier is already in Text.
static struct ASTnode *postfix(void) {
  struct ASTnode *n;
  int id;

  // Scan in the next token to see if we have a postfix expression
  scan(&Token);

  // Function call
  if (Token.token == T_LPAREN)
    return (funccall());

  // An array reference
  if (Token.token == T_LBRACKET)
    return (array_access());

  // A variable. Check that the variable exists.
  id = findsymbol(Text);
  if (id == -1 || Symtable[id].stype != S_VARIABLE)
    fatals("Unknown variable", Text);

  switch (Token.token) {
      // Post-increment: skip over the token
    case T_INC:
      scan(&Token);
      n = mkastleaf(A_POSTINC, Symtable[id].type, id);
      break;

      // Post-decrement: skip over the token
    case T_DEC:
      scan(&Token);
      n = mkastleaf(A_POSTDEC, Symtable[id].type, id);
      break;

      // Just a variable reference
    default:
      n = mkastleaf(A_IDENT, Symtable[id].type, id);
  }
  return (n);
}

// Parse a primary factor and return an
// AST node representing it.
static struct ASTnode *primary(void) {
  struct ASTnode *n;
  int id;

  switch (Token.token) {
    case T_INTLIT:
      // For an INTLIT token, make a leaf AST node for it.
      // Make it a P_CHAR if it's within the P_CHAR range
      if ((Token.intvalue) >= 0 && (Token.intvalue < 256))
	n = mkastleaf(A_INTLIT, P_CHAR, Token.intvalue);
      else
	n = mkastleaf(A_INTLIT, P_INT, Token.intvalue);
      break;

    case T_STRLIT:
      // For a STRLIT token, generate the assembly for it.
      // Then make a leaf AST node for it. id is the string's label.
      id = genglobstr(Text);
      n = mkastleaf(A_STRLIT, P_CHARPTR, id);
      break;

    case T_IDENT:
      return (postfix());

    case T_LPAREN:
      // Beginning of a parenthesised expression, skip the '('.
      // Scan in the expression and the right parenthesis
      scan(&Token);
      n = binexpr(0);
      rparen();
      return (n);

    default:
      fatald("Expecting a primary expression, got token", Token.token);
  }

  // Scan in the next token and return the leaf node
  scan(&Token);
  return (n);
}

// Convert a binary operator token into a binary AST operation.
// We rely on a 1:1 mapping from token to AST operation
static int binastop(int tokentype) {
  if (tokentype > T_EOF && tokentype <= T_SLASH)
    return (tokentype);
  fatald("Syntax error, token", tokentype);
  return (0);			// Keep -Wall happy
}

// Return true if a token is right-associative,
// false otherwise.
static int rightassoc(int tokentype) {
  if (tokentype == T_ASSIGN)
    return (1);
  return (0);
}

// Operator precedence for each token. Must
// match up with the order of tokens in defs.h
static int OpPrec[] = {
  0, 10, 20, 30,		// T_EOF, T_ASSIGN, T_LOGOR, T_LOGAND
  40, 50, 60,			// T_OR, T_XOR, T_AMPER 
  70, 70,			// T_EQ, T_NE
  80, 80, 80, 80,		// T_LT, T_GT, T_LE, T_GE
  90, 90,			// T_LSHIFT, T_RSHIFT
  100, 100,			// T_PLUS, T_MINUS
  110, 110			// T_STAR, T_SLASH
};

// Check that we have a binary operator and
// return its precedence.
static int op_precedence(int tokentype) {
  int prec;
  if (tokentype > T_SLASH)
    fatald("Token with no precedence in op_precedence:", tokentype);
  prec = OpPrec[tokentype];
  if (prec == 0)
    fatald("Syntax error, token", tokentype);
  return (prec);
}

// prefix_expression: primary
//     | '*'  prefix_expression
//     | '&'  prefix_expression
//     | '-'  prefix_expression
//     | '++' prefix_expression
//     | '--' prefix_expression
//     ;

// Parse a prefix expression and return 
// a sub-tree representing it.
struct ASTnode *prefix(void) {
  struct ASTnode *tree;
  switch (Token.token) {
    case T_AMPER:
      // Get the next token and parse it
      // recursively as a prefix expression
      scan(&Token);
      tree = prefix();

      // Ensure that it's an identifier
      if (tree->op != A_IDENT)
	fatal("& operator must be followed by an identifier");

      // Now change the operator to A_ADDR and the type to
      // a pointer to the original type
      tree->op = A_ADDR;
      tree->type = pointer_to(tree->type);
      break;
    case T_STAR:
      // Get the next token and parse it
      // recursively as a prefix expression
      scan(&Token);
      tree = prefix();

      // For now, ensure it's either another deref or an
      // identifier
      if (tree->op != A_IDENT && tree->op != A_DEREF)
	fatal("* operator must be followed by an identifier or *");

      // Prepend an A_DEREF operation to the tree
      tree = mkastunary(A_DEREF, value_at(tree->type), tree, 0);
      break;
    case T_MINUS:
      // Get the next token and parse it
      // recursively as a prefix expression
      scan(&Token);
      tree = prefix();

      // Prepend a A_NEGATE operation to the tree and
      // make the child an rvalue. Because chars are unsigned,
      // also widen this to int so that it's signed
      tree->rvalue = 1;
      tree = modify_type(tree, P_INT, 0);
      tree = mkastunary(A_NEGATE, tree->type, tree, 0);
      break;
    case T_INVERT:
      // Get the next token and parse it
      // recursively as a prefix expression
      scan(&Token);
      tree = prefix();

      // Prepend a A_INVERT operation to the tree and
      // make the child an rvalue.
      tree->rvalue = 1;
      tree = mkastunary(A_INVERT, tree->type, tree, 0);
      break;
    case T_LOGNOT:
      // Get the next token and parse it
      // recursively as a prefix expression
      scan(&Token);
      tree = prefix();

      // Prepend a A_LOGNOT operation to the tree and
      // make the child an rvalue.
      tree->rvalue = 1;
      tree = mkastunary(A_LOGNOT, tree->type, tree, 0);
      break;
    case T_INC:
      // Get the next token and parse it
      // recursively as a prefix expression
      scan(&Token);
      tree = prefix();

      // For now, ensure it's an identifier
      if (tree->op != A_IDENT)
	fatal("++ operator must be followed by an identifier");

      // Prepend an A_PREINC operation to the tree
      tree = mkastunary(A_PREINC, tree->type, tree, 0);
      break;
    case T_DEC:
      // Get the next token and parse it
      // recursively as a prefix expression
      scan(&Token);
      tree = prefix();

      // For now, ensure it's an identifier
      if (tree->op != A_IDENT)
	fatal("-- operator must be followed by an identifier");

      // Prepend an A_PREDEC operation to the tree
      tree = mkastunary(A_PREDEC, tree->type, tree, 0);
      break;
    default:
      tree = primary();
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
  left = prefix();

  // If we hit a semicolon or ')', return just the left node
  tokentype = Token.token;
  if (tokentype == T_SEMI || tokentype == T_RPAREN || tokentype == T_RBRACKET) {
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

    if (ASTop == A_ASSIGN) {
      // Assignment
      // Make the right tree into an rvalue
      right->rvalue = 1;

      // Ensure the right's type matches the left
      right = modify_type(right, left->type, 0);
      if (left == NULL)
	fatal("Incompatible expression in assignment");

      // Make an assignment AST tree. However, switch
      // left and right around, so that the right expression's 
      // code will be generated before the left expression
      ltemp = left;
      left = right;
      right = ltemp;
    } else {

      // We are not doing an assignment, so both trees should be rvalues
      // Convert both trees into rvalue if they are lvalue trees
      left->rvalue = 1;
      right->rvalue = 1;

      // Ensure the two types are compatible by trying
      // to modify each tree to match the other's type.
      ltemp = modify_type(left, right->type, ASTop);
      rtemp = modify_type(right, left->type, ASTop);
      if (ltemp == NULL && rtemp == NULL)
	fatal("Incompatible types in binary expression");
      if (ltemp != NULL)
	left = ltemp;
      if (rtemp != NULL)
	right = rtemp;
    }

    // Join that sub-tree with ours. Convert the token
    // into an AST operation at the same time.
    left = mkastnode(binastop(tokentype), left->type, left, NULL, right, 0);

    // Update the details of the current token.
    // If we hit a semicolon, ')' or ']', return just the left node
    tokentype = Token.token;
    if (tokentype == T_SEMI || tokentype == T_RPAREN
	|| tokentype == T_RBRACKET) {
      left->rvalue = 1;
      return (left);
    }
  }

  // Return the tree we have when the precedence
  // is the same or lower
  left->rvalue = 1;
  return (left);
}
