#include "defs.h"
#include "misc.h"

// Lexical scanning
// Copyright (c) 2019 Warren Toomey, GPL3

int Line = 1;                   // Current line number
int Newlinenum=0;		// Flag: has line number changed
int Linestart = 1;              // True if at start of a line
int Putback = '\n';             // Character put back by scanner
char *Infilename;               // Name of file we are parsing
int Newfilename=0;		// Flag: has filename changed
FILE *Infile;                   // Input file struct
struct token Token;             // Last token scanned
struct token Peektoken;         // A look-ahead token
char Text[TEXTLEN + 1];         // Last identifier scanned

int scan(struct token *t, int nocpp);

// Return the position of character c
// in string s, or -1 if c not found
static int chrpos(char *s, int c) {
  int i;
  for (i = 0; s[i] != '\0'; i++)
    if (s[i] == (char) c)
      return (i);
  return (-1);
}

// Get the next character from the input file.
static int next(void) {
  int c, l;

  if (Putback) {		// Use the character put
    c = Putback;		// back if there is one
    Putback = 0;
    return (c);
  }

  c = fgetc(Infile);		// Read from input file

  while (Linestart && c == '#') {	// We've hit a pre-processor statement
    Linestart = 0;		// No longer at the start of the line
    scan(&Token, 1);		// Get the line number into l
    if (Token.token != T_INTLIT)
      fatals("Expecting pre-processor line number, got:", Text);
    l = Token.intvalue;

    scan(&Token, 1);		// Get the filename in Text
    if (Token.token != T_STRLIT)
      fatals("Expecting pre-processor file name, got:", Text);

    if (Text[0] != '<') {	// If this is a real filename
      if (strcmp(Text, Infilename)) {	// and not the one we have now
	free(Infilename);
	Infilename = strdup(Text);	// save it. Then update the line num
        Newfilename=1;
      }
      Line = l;
      Newlinenum=1;
    }

    while ((c = fgetc(Infile)) != '\n');	// Skip to the end of the line
    c = fgetc(Infile);		// and get the next character
    Linestart = 1;		// Now back at the start of the line
  }

  Linestart = 0;		// No longer at the start of the line
  if ('\n' == c) {
    Line++;			// Increment line count
    Newlinenum=1;
    Linestart = 1;		// Now back at the start of the line
  }
  return (c);
}

// Put back an unwanted character
static void putback(int c) {
  Putback = c;
}

// Skip past input that we don't need to deal with, 
// i.e. whitespace, newlines. Return the first
// character we do need to deal with.
static int skip(void) {
  int c;

  c = next();
  while (' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c) {
    c = next();
  }
  return (c);
}

// Read in a hexadecimal constant from the input
static int hexchar(void) {
  int c, h, n = 0, f = 0;

  // Loop getting characters
  while (isxdigit(c = next())) {
    // Convert from char to int value
    h = chrpos("0123456789abcdef", tolower(c));

    // Add to running hex value
    n = n * 16 + h;
    f = 1;
  }

  // We hit a non-hex character, put it back
  putback(c);

  // Flag tells us we never saw any hex characters
  if (!f)
    fatal("missing digits after '\\x'");
  if (n > 255)
    fatal("value out of range after '\\x'");

  return (n);
}

// Return the next character from a character
// or string literal. Also return if this
// character was quoted with a preceding backslash
static int scanch(int *slash) {
  int i, c, c2;
  *slash=0;

  // Get the next input character and interpret
  // metacharacters that start with a backslash
  c = next();
  if (c == '\\') {
    *slash=1;
    switch (c = next()) {
      case 'a':
	return ('\a');
      case 'b':
	return ('\b');
      case 'f':
	return ('\f');
      case 'n':
	return ('\n');
      case 'r':
	return ('\r');
      case 't':
	return ('\t');
      case 'v':
	return ('\v');
      case '\\':
	return ('\\');
      case '"':
	return ('"');
      case '\'':
	return ('\'');

	// Deal with octal constants by reading in
	// characters until we hit a non-octal digit.
	// Build up the octal value in c2 and count
	// # digits in i. Permit only 3 octal digits.
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
	for (i = c2 = 0; isdigit(c) && c < '8'; c = next()) {
	  if (++i > 3)
	    break;
	  c2 = c2 * 8 + (c - '0');
	}

	putback(c);		// Put back the first non-octal char
	return (c2);
      case 'x':
	return (hexchar());
      default:
        fatalc("unknown escape sequence", c);
    }
  }
  return (c);			// Just an ordinary old character!
}

// Scan and return an integer literal
// value from the input file.
static int scanint(int c) {
  int k, val = 0, radix = 10;

  // Assume the radix is 10, but if it starts with 0
  if (c == '0') {
    // and the next character is 'x', it's radix 16
    if ((c = next()) == 'x') {
      radix = 16;
      c = next();
    } else
      // Otherwise, it's radix 8
      radix = 8;

  }
  // Convert each character into an int value
  while ((k = chrpos("0123456789abcdef", tolower(c))) >= 0) {
    if (k >= radix)
      fatalc("invalid digit in integer literal", c);
    val = val * radix + k;
    c = next();
  }

  // We hit a non-integer character, put it back.
  putback(c);
  return (val);
}

// Scan in a string literal from the input file,
// and store it in buf[]. Return the length of
// the string. 
static int scanstr(char *buf) {
  int i, c;
  int slash;

  // Loop while we have enough buffer space
  for (i = 0; i < TEXTLEN - 1; i++) {
    // Get the next char and append to buf
    // Return when we hit the ending double quote
    // (which wasn't quoted with a backslash)
    c = scanch(&slash);
    if (c == '"' && slash==0) {
      buf[i] = 0;
      return (i);
    }
    buf[i] = (char) c;
  }

  // Ran out of buf[] space
  fatal("String literal too long");
  return (0);
}

// Scan an identifier from the input file and
// store it in buf[]. Return the identifier's length
static int scanident(int c, char *buf, int lim) {
  int i = 0;

  // Allow digits, alpha and underscores
  while (isalpha(c) || isdigit(c) || '_' == c) {
    // Error if we hit the identifier length limit,
    // else append to buf[] and get next character
    if (lim - 1 == i) {
      fatal("Identifier too long");
    } else if (i < lim - 1) {
      buf[i++] = (char) c;
    }
    c = next();
  }

  // We hit a non-valid character, put it back.
  // NUL-terminate the buf[] and return the length
  putback(c);
  buf[i] = '\0';
  return (i);
}

// Given a word from the input, return the matching
// keyword token number or 0 if it's not a keyword.
// Switch on the first letter so that we don't have
// to waste time strcmp()ing against all the keywords.
static int keyword(char *s) {
  switch (*s) {
    case 'b':
      if (!strcmp(s, "break"))
	return (T_BREAK);
      break;
    case 'c':
      if (!strcmp(s, "case"))
	return (T_CASE);
      if (!strcmp(s, "char"))
	return (T_CHAR);
      if (!strcmp(s, "continue"))
	return (T_CONTINUE);
      break;
    case 'd':
      if (!strcmp(s, "default"))
	return (T_DEFAULT);
      break;
    case 'e':
      if (!strcmp(s, "else"))
	return (T_ELSE);
      if (!strcmp(s, "enum"))
	return (T_ENUM);
      if (!strcmp(s, "extern"))
	return (T_EXTERN);
      break;
    case 'f':
      if (!strcmp(s, "for"))
	return (T_FOR);
      break;
    case 'i':
      if (!strcmp(s, "if"))
	return (T_IF);
      if (!strcmp(s, "int"))
	return (T_INT);
      break;
    case 'l':
      if (!strcmp(s, "long"))
	return (T_LONG);
      break;
    case 'r':
      if (!strcmp(s, "return"))
	return (T_RETURN);
      break;
    case 's':
      if (!strcmp(s, "sizeof"))
	return (T_SIZEOF);
      if (!strcmp(s, "static"))
	return (T_STATIC);
      if (!strcmp(s, "struct"))
	return (T_STRUCT);
      if (!strcmp(s, "switch"))
	return (T_SWITCH);
      break;
    case 't':
      if (!strcmp(s, "typedef"))
	return (T_TYPEDEF);
      break;
    case 'u':
      if (!strcmp(s, "union"))
	return (T_UNION);
      break;
    case 'v':
      if (!strcmp(s, "void")) {
	return (T_VOID);
}
      break;
    case 'w':
      if (!strcmp(s, "while"))
	return (T_WHILE);
      break;
  }
  return (0);
}

#ifdef DEBUG
// List of token strings, for debugging purposes
char *Tstring[] = {
  "EOF", "=", "+=", "-=", "*=", "/=", "%=",
  "?", "||", "&&", "|", "^", "&",
  "==", "!=", "<", ">", "<=", ">=", "<<", ">>",
  "+", "-", "*", "/", "%", "++", "--", "~", "!",
  "void", "char", "int", "long",
  "if", "else", "while", "for", "return",
  "struct", "union", "enum", "typedef",
  "extern", "break", "continue", "switch",
  "case", "default", "sizeof", "static",
  "intlit", "strlit", ";", "identifier",
  "{", "}", "(", ")", "[", "]", ",", ".",
  "->", ":", "...", "charlit", "filename", "linenum"
};
#endif

// Scan and return the next token found in the input.
// Return 1 if token valid, 0 if no tokens left.
// If nocpp, don't return on new filenames or line numbers.
// This is because we use scan() when parsing new filenames
// and line numbers :-)
int scan(struct token *t, int nocpp) {
  int c, tokentype;
  int slash;

  // Skip whitespace
  c = skip();

  if (nocpp==0) {
    // If the filename changed, return the filename
    if (Newfilename) {
      t->token = T_FILENAME;
      Newfilename=0;
      putback(c);
      return (1);
    }

    // If the line number changed, return the line number
    if (Newlinenum) {
      t->intvalue = Line;
      t->token = T_LINENUM;
      Newlinenum=0;
      putback(c);
      return (1);
    }
  }

  // If we have a lookahead token, return this token
  if (Peektoken.token != 0) {
    t->token = Peektoken.token;
    t->intvalue = Peektoken.intvalue;
    Peektoken.token = 0;
    return (1);
  }

  // Determine the token based on
  // the input character
  switch (c) {
    case EOF:
      t->token = T_EOF;
      return (0);
    case '+':
      if ((c = next()) == '+') {
	t->token = T_INC;
      } else if (c == '=') {
	t->token = T_ASPLUS;
      } else {
	putback(c);
	t->token = T_PLUS;
      }
      break;
    case '-':
      if ((c = next()) == '-') {
	t->token = T_DEC;
      } else if (c == '>') {
	t->token = T_ARROW;
      } else if (c == '=') {
	t->token = T_ASMINUS;
      } else if (isdigit(c)) {	// Negative int literal
	t->intvalue = -scanint(c);
	t->token = T_INTLIT;
      } else {
	putback(c);
	t->token = T_MINUS;
      }
      break;
    case '*':
      if ((c = next()) == '=') {
	t->token = T_ASSTAR;
      } else {
	putback(c);
	t->token = T_STAR;
      }
      break;
    case '/':
      if ((c = next()) == '=') {
	t->token = T_ASSLASH;
      } else {
	putback(c);
	t->token = T_SLASH;
      }
      break;
    case '%':
      if ((c = next()) == '=') {
	t->token = T_ASMOD;
      } else {
	putback(c);
	t->token = T_MOD;
      }
      break;
    case ';':
      t->token = T_SEMI;
      break;
    case '{':
      t->token = T_LBRACE;
      break;
    case '}':
      t->token = T_RBRACE;
      break;
    case '(':
      t->token = T_LPAREN;
      break;
    case ')':
      t->token = T_RPAREN;
      break;
    case '[':
      t->token = T_LBRACKET;
      break;
    case ']':
      t->token = T_RBRACKET;
      break;
    case '~':
      t->token = T_INVERT;
      break;
    case '^':
      t->token = T_XOR;
      break;
    case ',':
      t->token = T_COMMA;
      break;
    case '.':
      if ((c = next()) == '.') {
        t->token = T_ELLIPSIS;
        if ((c = next()) != '.')
	  fatal("Expected '...', only got '..'\n");
      } else {
	putback(c);
        t->token = T_DOT;
      }
      break;
    case ':':
      t->token = T_COLON;
      break;
    case '?':
      t->token = T_QUESTION;
      break;
    case '=':
      if ((c = next()) == '=') {
	t->token = T_EQ;
      } else {
	putback(c);
	t->token = T_ASSIGN;
      }
      break;
    case '!':
      if ((c = next()) == '=') {
	t->token = T_NE;
      } else {
	putback(c);
	t->token = T_LOGNOT;
      }
      break;
    case '<':
      if ((c = next()) == '=') {
	t->token = T_LE;
      } else if (c == '<') {
	t->token = T_LSHIFT;
      } else {
	putback(c);
	t->token = T_LT;
      }
      break;
    case '>':
      if ((c = next()) == '=') {
	t->token = T_GE;
      } else if (c == '>') {
	t->token = T_RSHIFT;
      } else {
	putback(c);
	t->token = T_GT;
      }
      break;
    case '&':
      if ((c = next()) == '&') {
	t->token = T_LOGAND;
      } else {
	putback(c);
	t->token = T_AMPER;
      }
      break;
    case '|':
      if ((c = next()) == '|') {
	t->token = T_LOGOR;
      } else {
	putback(c);
	t->token = T_OR;
      }
      break;
    case '\'':
      // If it's a quote, scan in the
      // literal character value and
      // the trailing quote
      t->intvalue = scanch(&slash);
      t->token = T_CHARLIT;
      if (next() != '\'')
	fatal("Expected '\\'' at end of char literal");
      break;
    case '"':
      // Scan in a literal string
      scanstr(Text);
      t->token = T_STRLIT;
      break;
    default:
      // If it's a digit, scan the
      // literal integer value in
      if (isdigit(c)) {
	t->intvalue = scanint(c);
	t->token = T_INTLIT;
	break;
      } else if (isalpha(c) || '_' == c) {
	// Read in a keyword or identifier
	scanident(c, Text, TEXTLEN);

	// If it's a recognised keyword, return that token
	if ((tokentype = keyword(Text)) != 0) {
	  t->token = tokentype;
	  break;
	}
	// Not a recognised keyword, so it must be an identifier
	t->token = T_IDENT;
	break;
      }
      // The character isn't part of any recognised token, error
      fatalc("Unrecognised character", c);
  }

  // We found a token
  return (1);
}

// Read lines of code from stdin and output
// a token stream
int main() {
  int i;

  Infile= stdin;
  Infilename = strdup("");	// Cpp hasn't told us the filename yet
  Peektoken.token = 0;          // Set there is no lookahead token
  scan(&Token, 0);              // Get the first token from the input

  // Loop getting more tokens
  while (Token.token != T_EOF) {

    // Output a binary stream of tokens to standard output.
    // T_INTLIT tokens are followed by the n-byte literal value.
    // T_STRLIT and T_IDENT tokens are followed by a NUL-terminated string.
    fputc(Token.token, stdout);
    switch (Token.token) {
    case T_INTLIT:
    case T_CHARLIT:
      i= Token.intvalue;
      fwrite(&i, sizeof(int), 1, stdout);
      // fprintf(stderr, "%02X: %d\n", Token.token, Token.intvalue);
      break;
    case T_STRLIT:
      fputs(Text, stdout);
      fputc(0, stdout);
      // fprintf(stderr, "%02X: \"%s\"\n", Token.token, Text);
      break;
    case T_IDENT:
      fputs(Text, stdout);
      fputc(0, stdout);
      // fprintf(stderr, "%02X: %s\n", Token.token, Text);
      break;
    case T_FILENAME:
      fputs(Infilename, stdout);
      fputc(0, stdout);
      // fprintf(stderr, "%02X: %s\n", Token.token, Infilename);
      break;
    case T_LINENUM:
      fwrite(&Line, sizeof(int), 1, stdout);
      // fprintf(stderr, "%02X: %d\n", Token.token, Line);
      break;
    default:
      // fprintf(stderr, "%02X: %s\n", Token.token, Tstring[Token.token]);
    }
    scan(&Token, 0);
  }
  
  exit(0);
  return(0);
}
