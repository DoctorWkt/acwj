#include <stdio.h>
#include <unistd.h>
#include "defs.h"
#include "data.h"
#include "parse.h"

// Miscellaneous functions
// Copyright (c) 2019 Warren Toomey, GPL3

// Print out fatal messages
void fatal(char *s) {
  fprintf(stderr, "%s on line %d of %s\n", s, Line, Infilename);
  exit(1);
}

void fatals(char *s1, char *s2) {
  fprintf(stderr, "%s:%s on line %d of %s\n", s1, s2, Line, Infilename);
  exit(1);
}

void fatald(char *s, int d) {
  fprintf(stderr, "%s:%d on line %d of %s\n", s, d, Line, Infilename);
  exit(1);
}

void fatalc(char *s, int c) {
  fprintf(stderr, "%s:%c on line %d of %s\n", s, c, Line, Infilename);
  exit(1);
}

// Read at most count-1 characters from the
// f FILE and store them in the s buffer.
// Terminate the s buffer with a NUL.
// Return NULL if unable to read or an EOF.
// Else, return the original s pointer pointer.
char *fgetstr(char *s, size_t count, FILE * f) {
  size_t i = count;
  size_t err;
  char ch;
  char *ret = s;

  while (i-- != 0) {
    err= fread(&ch, 1, 1, f);
    if (err!=1) {
      if (s == ret)
        return(NULL);
      break;
    }
    *s++ = ch;
    if (ch == 0)
      break;
  }
  *s = 0;
  return(ferror(f) ? (char *) NULL : ret);
}
