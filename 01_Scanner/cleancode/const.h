#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int Line;
extern int Putback;
extern FILE* Infile;

typedef struct token{
    int type_idx;
    int value;
}Token;

enum {
  T_PLUS, T_MINUS, T_STAR, T_SLASH, T_INTLIT
};

extern int scan(Token*);

static char const * const token_str[] = { "+", "-", "*", "/", "intlit" };
