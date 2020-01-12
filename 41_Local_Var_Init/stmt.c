#include "defs.h"
#include "data.h"
#include "decl.h"

// Parsing of statements
// Copyright (c) 2019 Warren Toomey, GPL3

// Prototypes
static struct ASTnode *single_statement(void);

// compound_statement:          // empty, i.e. no statement
//      |      statement
//      |      statement statements
//      ;
//
// statement: declaration
//      |     expression_statement
//      |     function_call
//      |     if_statement
//      |     while_statement
//      |     for_statement
//      |     return_statement
//      ;


// if_statement: if_head
//      |        if_head 'else' statement
//      ;
//
// if_head: 'if' '(' true_false_expression ')' statement  ;
//
// Parse an IF statement including any
// optional ELSE clause and return its AST
static struct ASTnode *if_statement(void) {
  struct ASTnode *condAST, *trueAST, *falseAST = NULL;

  // Ensure we have 'if' '('
  match(T_IF, "if");
  lparen();

  // Parse the following expression
  // and the ')' following. Force a
  // non-comparison to be boolean
  // the tree's operation is a comparison.
  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    condAST = mkastunary(A_TOBOOL, condAST->type, condAST, NULL, 0);
  rparen();

  // Get the AST for the statement
  trueAST = single_statement();

  // If we have an 'else', skip it
  // and get the AST for the statement
  if (Token.token == T_ELSE) {
    scan(&Token);
    falseAST = single_statement();
  }
  // Build and return the AST for this statement
  return (mkastnode(A_IF, P_NONE, condAST, trueAST, falseAST, NULL, 0));
}


// while_statement: 'while' '(' true_false_expression ')' statement  ;
//
// Parse a WHILE statement and return its AST
static struct ASTnode *while_statement(void) {
  struct ASTnode *condAST, *bodyAST;

  // Ensure we have 'while' '('
  match(T_WHILE, "while");
  lparen();

  // Parse the following expression
  // and the ')' following. Force a
  // non-comparison to be boolean
  // the tree's operation is a comparison.
  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    condAST = mkastunary(A_TOBOOL, condAST->type, condAST, NULL, 0);
  rparen();

  // Get the AST for the statement.
  // Update the loop depth in the process
  Looplevel++;
  bodyAST = single_statement();
  Looplevel--;

  // Build and return the AST for this statement
  return (mkastnode(A_WHILE, P_NONE, condAST, NULL, bodyAST, NULL, 0));
}

// for_statement: 'for' '(' expression_list ';'
//                          true_false_expression ';'
//                          expression_list ')' statement  ;
//
// Parse a FOR statement and return its AST
static struct ASTnode *for_statement(void) {
  struct ASTnode *condAST, *bodyAST;
  struct ASTnode *preopAST, *postopAST;
  struct ASTnode *tree;

  // Ensure we have 'for' '('
  match(T_FOR, "for");
  lparen();

  // Get the pre_op expression and the ';'
  preopAST = expression_list(T_SEMI);
  semi();

  // Get the condition and the ';'.
  // Force a non-comparison to be boolean
  // the tree's operation is a comparison.
  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    condAST = mkastunary(A_TOBOOL, condAST->type, condAST, NULL, 0);
  semi();

  // Get the post_op expression and the ')'
  postopAST = expression_list(T_RPAREN);
  rparen();

  // Get the statement which is the body
  // Update the loop depth in the process
  Looplevel++;
  bodyAST = single_statement();
  Looplevel--;

  // Glue the statement and the postop tree
  tree = mkastnode(A_GLUE, P_NONE, bodyAST, NULL, postopAST, NULL, 0);

  // Make a WHILE loop with the condition and this new body
  tree = mkastnode(A_WHILE, P_NONE, condAST, NULL, tree, NULL, 0);

  // And glue the preop tree to the A_WHILE tree
  return (mkastnode(A_GLUE, P_NONE, preopAST, NULL, tree, NULL, 0));
}

// return_statement: 'return' '(' expression ')'  ;
//
// Parse a return statement and return its AST
static struct ASTnode *return_statement(void) {
  struct ASTnode *tree;

  // Can't return a value if function returns P_VOID
  if (Functionid->type == P_VOID)
    fatal("Can't return from a void function");

  // Ensure we have 'return' '('
  match(T_RETURN, "return");
  lparen();

  // Parse the following expression
  tree = binexpr(0);

  // Ensure this is compatible with the function's type
  tree = modify_type(tree, Functionid->type, 0);
  if (tree == NULL)
    fatal("Incompatible type to return");

  // Add on the A_RETURN node
  tree = mkastunary(A_RETURN, P_NONE, tree, NULL, 0);

  // Get the ')' and ';'
  rparen();
  semi();
  return (tree);
}

// break_statement: 'break' ;
//
// Parse a break statement and return its AST
static struct ASTnode *break_statement(void) {

  if (Looplevel == 0 && Switchlevel == 0)
    fatal("no loop or switch to break out from");
  scan(&Token);
  semi();
  return (mkastleaf(A_BREAK, 0, NULL, 0));
}

// continue_statement: 'continue' ;
//
// Parse a continue statement and return its AST
static struct ASTnode *continue_statement(void) {

  if (Looplevel == 0)
    fatal("no loop to continue to");
  scan(&Token);
  semi();
  return (mkastleaf(A_CONTINUE, 0, NULL, 0));
}

// Parse a switch statement and return its AST
static struct ASTnode *switch_statement(void) {
  struct ASTnode *left, *n, *c, *casetree= NULL, *casetail;
  int inloop=1, casecount=0;
  int seendefault=0;
  int ASTop, casevalue;

  // Skip the 'switch' and '('
  scan(&Token);
  lparen();

  // Get the switch expression, the ')' and the '{'
  left= binexpr(0);
  rparen();
  lbrace();

  // Ensure that this is of int type
  if (!inttype(left->type))
    fatal("Switch expression is not of integer type");

  // Build an A_SWITCH subtree with the expression as
  // the child
  n= mkastunary(A_SWITCH, 0, left, NULL, 0);

  // Now parse the cases
  Switchlevel++;
  while (inloop) {
    switch(Token.token) {
      // Leave the loop when we hit a '}'
      case T_RBRACE: if (casecount==0)
			fatal("No cases in switch");
		     inloop=0; break;
      case T_CASE:
      case T_DEFAULT:
	// Ensure this isn't after a previous 'default'
	if (seendefault)
	  fatal("case or default after existing default");

	// Set the AST operation. Scan the case value if required
	if (Token.token==T_DEFAULT) {
	  ASTop= A_DEFAULT; seendefault= 1; scan(&Token);
	} else  {
	  ASTop= A_CASE; scan(&Token);
	  left= binexpr(0);
	  // Ensure the case value is an integer literal
	  if (left->op != A_INTLIT)
	    fatal("Expecting integer literal for case value");
	  casevalue= left->intvalue;

	  // Walk the list of existing case values to ensure
  	  // that there isn't a duplicate case value
	  for (c= casetree; c != NULL; c= c -> right)
	    if (casevalue == c->intvalue)
	      fatal("Duplicate case value");
        }

	// Scan the ':' and get the compound expression
	match(T_COLON, ":");
	left= compound_statement(1); casecount++;

	// Build a sub-tree with the compound statement as the left child
	// and link it in to the growing A_CASE tree
	if (casetree==NULL) {
	  casetree= casetail= mkastunary(ASTop, 0, left, NULL, casevalue);
	} else {
	  casetail->right= mkastunary(ASTop, 0, left, NULL, casevalue);
	  casetail= casetail->right;
	}
	break;
      default:
        fatals("Unexpected token in switch", Token.tokstr);
    }
  }
  Switchlevel--;

  // We have a sub-tree with the cases and any default. Put the
  // case count into the A_SWITCH node and attach the case tree.
  n->intvalue= casecount;
  n->right= casetree;
  rbrace();

  return(n);
}

// Parse a single statement and return its AST.
static struct ASTnode *single_statement(void) {
  struct ASTnode *stmt;
  struct symtable *ctype;

  switch (Token.token) {
    case T_LBRACE:
      // We have a '{', so this is a compound statement
      lbrace();
      stmt = compound_statement(0);
      rbrace();
      return(stmt);
    case T_IDENT:
      // We have to see if the identifier matches a typedef.
      // If not, treat it as an expression.
      // Otherwise, fall down to the parse_type() call.
      if (findtypedef(Text) == NULL) {
	stmt= binexpr(0); semi(); return(stmt);
      }
    case T_CHAR:
    case T_INT:
    case T_LONG:
    case T_STRUCT:
    case T_UNION:
    case T_ENUM:
    case T_TYPEDEF:
      // The beginning of a variable declaration list.
      declaration_list(&ctype, C_LOCAL, T_SEMI, T_EOF, &stmt);
      semi();
      return (stmt);		// Any assignments from the declarations
    case T_IF:
      return (if_statement());
    case T_WHILE:
      return (while_statement());
    case T_FOR:
      return (for_statement());
    case T_RETURN:
      return (return_statement());
    case T_BREAK:
      return (break_statement());
    case T_CONTINUE:
      return (continue_statement());
    case T_SWITCH:
      return (switch_statement());
    default:
      // For now, see if this is an expression.
      // This catches assignment statements.
      stmt= binexpr(0); semi(); return(stmt);
  }
  return (NULL);		// Keep -Wall happy
}

// Parse a compound statement
// and return its AST. If inswitch is true,
// we look for a '}', 'case' or 'default' token
// to end the parsing. Otherwise, look for
// just a '}' to end the parsing.
struct ASTnode *compound_statement(int inswitch) {
  struct ASTnode *left = NULL;
  struct ASTnode *tree;

  while (1) {
    // Parse a single statement
    tree = single_statement();

    // For each new tree, either save it in left
    // if left is empty, or glue the left and the
    // new tree together
    if (tree != NULL) {
      if (left == NULL)
	left = tree;
      else
	left = mkastnode(A_GLUE, P_NONE, left, NULL, tree, NULL, 0);
    }

    // Leave if we've hit the end token
    if (Token.token == T_RBRACE) return(left);
    if (inswitch && (Token.token == T_CASE || Token.token == T_DEFAULT)) return(left);
  }
}
