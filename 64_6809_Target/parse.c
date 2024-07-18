#include "defs.h"
#define extern_
#include "data.h"
#undef extern_
#include "decl.h"
#include "gen.h"
#include "misc.h"
#include "sym.h"
#include "tree.h"

// C parser front-end.
// Copyright (c) 2023 Warren Toomey, GPL3

#ifdef DEBUG
void print_token(struct token *t) {
    switch (t->token) {
    case T_INTLIT:
    case T_CHARLIT:
      printf("%02X: %d\n", t->token, t->intvalue);
      break;
    case T_STRLIT:
      printf("%02X: \"%s\"\n", t->token, Text);
      break;
    case T_FILENAME:
      printf("%02X: filename \"%s\"\n", t->token, Text);
      break;
    case T_LINENUM:
      printf("%02X: linenum %d\n", t->token, t->intvalue);
      break;
    case T_IDENT:
      printf("%02X: %s\n", t->token, Text);
      break;
    default:
      printf("%02X: %s\n", t->token, Tstring[t->token]);
    }
}
#endif


// Scan and return the next token found in the input.
// Return 1 if token valid, 0 if no tokens left.
int scan(struct token *t) {
  int intvalue;

  // If we have a lookahead token, return this token
  if (Peektoken.token != 0) {
    t->token = Peektoken.token;
    t->tokstr = Peektoken.tokstr;
    t->intvalue = Peektoken.intvalue;
    Peektoken.token = 0;
#ifdef DEBUG
    print_token(t);
#endif
    return (1);
  }

  // We loop because we don't want to return
  // T_FILENAME or T_LINENUM tokens
  while (1) {
    t->token = fgetc(stdin);
    if (t->token == EOF) {
      t->token = T_EOF;
      break;
    }

    switch (t->token) {
    case T_LINENUM:
      fread(&Line, sizeof(int), 1, stdin);
      continue;
    case T_FILENAME:
      if (Infilename!=NULL) free(Infilename);
      fgetstr(Text, TEXTLEN + 1, stdin);
      Infilename= strdup(Text);
      continue;
    case T_INTLIT:
    case T_CHARLIT:
      fread(&intvalue, sizeof(int), 1, stdin);
      t->intvalue = intvalue;
      break;
    case T_STRLIT:
    case T_IDENT:
      fgetstr(Text, TEXTLEN + 1, stdin);
      break;
    }
#ifdef DEBUG
    print_token(t);
#endif
    return (1);
  }
  return(0);
}

// Ensure that the current token is t,
// and fetch the next token. Otherwise
// throw an error 
void match(int t, char *what) {
  if (Token.token == t) {
    scan(&Token);
  } else {
    fatals("Expected", what);
  }
}

// Match a semicolon and fetch the next token
void semi(void) {
  match(T_SEMI, ";");
}

// Match a left brace and fetch the next token
void lbrace(void) {
  match(T_LBRACE, "{");
}

// Match a right brace and fetch the next token
void rbrace(void) {
  match(T_RBRACE, "}");
}

// Match a left parenthesis and fetch the next token
void lparen(void) {
  match(T_LPAREN, "(");
}

// Match a right parenthesis and fetch the next token
void rparen(void) {
  match(T_RPAREN, ")");
}

// Match an identifier and fetch the next token
void ident(void) {
  match(T_IDENT, "identifier");
}

// Match a comma and fetch the next token
void comma(void) {
  match(T_COMMA, "comma");
}

// Seralise an AST to Outfile
void serialiseAST(struct ASTnode *tree) {
  if (tree==NULL) return;

  // Dump this node
  fwrite(tree, sizeof(struct ASTnode), 1, Outfile);

  // Dump any literal string/identifier
  if (tree->name!=NULL) {
    fputs(tree->name, Outfile);
    fputc(0, Outfile);
  }

  // Dump all the children
  serialiseAST(tree->left);
  serialiseAST(tree->mid);
  serialiseAST(tree->right);
}

// Parse the token stream on stdin
// and output serialised ASTs and
// a symbol table.
int main(int argc, char **argv) {

  if (argc <2 || argc >3) {
    fprintf(stderr, "Usage: %s symfile <astfile>\n", argv[0]);
    fprintf(stderr, "  ASTs on stdout if astfile not specified\n");
    exit(1);
  }

  if (argc==3) {
    Outfile= fopen(argv[2], "w");
    if (Outfile == NULL) {
      fprintf(stderr, "Can't create %s\n", argv[2]); exit(1);
    }
  } else 
    Outfile= stdout;

  Symfile= fopen(argv[1], "w+");
  if (Symfile == NULL) {
    fprintf(stderr, "Can't create %s\n", argv[1]); exit(1);
  }

  freeSymtable();		// Clear the symbol table
  scan(&Token);                 // Get the first token from the input
  Peektoken.token = 0;		// and set there is no lookahead token
  global_declarations();        // Parse the global declarations
  flushSymtable();		// Flush any residual symbols
  fclose(Symfile);
  exit(0);
  return(0);
}
