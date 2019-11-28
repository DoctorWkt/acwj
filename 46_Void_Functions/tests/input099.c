#include <stdio.h>

// List of token strings, for debugging purposes.
// As yet, we can't store a NULL into the list
char *Tstring[] = {
  "EOF", "=", "||", "&&", "|", "^", "&",
  "==", "!=", ",", ">", "<=", ">=", "<<", ">>",
  "+", "-", "*", "/", "++", "--", "~", "!",
  "void", "char", "int", "long",
  "if", "else", "while", "for", "return",
  "struct", "union", "enum", "typedef",
  "extern", "break", "continue", "switch",
  "case", "default",
  "intlit", "strlit", ";", "identifier",
  "{", "}", "(", ")", "[", "]", ",", ".",
  "->", ":", ""
};

int main() {
  int i;
  char *str;

  i=0;
  while (1) {
    str= Tstring[i];
    if (*str == 0) break;
    printf("%s\n", str);
    i++;
  }
  return(0);
}
