#include "defs.h"

// C Lexical scanning: detokeniser
// Copyright (c) 2023 Warren Toomey, GPL3

char Text[TEXTLEN + 1];		// Last identifier scanned
extern char *Tstring[];

// Read at most count-1 characters from the
// f FILE and store them in the s buffer.
// Terminate the s buffer with a NUL.
// Return NULL if unable to read or an EOF.
// Else, return the original s pointer pointer.
char *fgetstr(char *s, size_t count, FILE * f) {
  size_t i = count;
  int ch;
  char *ret = s;

  while (i-- != 0) {
    if ((ch = getc(f)) == EOF) {
      if (s == ret)
	return(NULL);
      break;
    }
    *s++ = (char) ch;
    if (ch == 0)
      break;
  }
  *s = 0;
  return(ferror(f) ? (char *) NULL : ret);
}

int main(int argc, char **argv) {
  FILE *in;
  int token;
  int intval;

  if (argc !=2) {
    fprintf(stderr, "Usage: %s tokenfile\n", argv[0]); exit(1);
  }

  in= fopen(argv[1], "r");
  if (in==NULL) {
    fprintf(stderr, "Unable to open %s\n", argv[1]); exit(1);
  }

  // Read until no more tokens left
  while (1) {
    token = fgetc(in);
    if (token == EOF)
      break;

    switch (token) {
    case T_INTLIT:
    case T_CHARLIT:
      fread(&intval, sizeof(int), 1, in);
      printf("%02X: %d\n", token, intval);
      break;
    case T_STRLIT:
      fgetstr(Text, TEXTLEN + 1, in);
      printf("%02X: \"%s\"\n", token, Text);
      break;
    case T_FILENAME:
      fgetstr(Text, TEXTLEN + 1, in);
      printf("%02X: filename \"%s\"\n", token, Text);
      break;
    case T_LINENUM:
      fread(&intval, sizeof(int), 1, in);
      printf("%02X: linenum %d\n", token, intval);
      break;
    case T_IDENT:
      fgetstr(Text, TEXTLEN + 1, in);
      printf("%02X: %s\n", token, Text);
      break;
    default:
      printf("%02X: %s\n", token, Tstring[token]);
    }
  }

  return (0);
}
