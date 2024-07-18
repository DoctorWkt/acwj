#include "defs.h"
#define extern_
#include "data.h"
#undef extern_

// Symbol table deserialiser
// Copyright (c) 2024 Warren Toomey, GPL3

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

// Read one symbol in. Return -1 if none.
int deserialiseSym(struct symtable *sym, FILE *in) {
  struct symtable *memb, *last;

  // Read one symbol struct in from in
  if (fread(sym, sizeof(struct symtable), 1, in)!=1)
    return(-1);

  // If the type is P_NONE, skip this one and read in
  // another symbol. This marks the division between
  // AST trees
  if (sym->type==P_NONE) {
    // Debug: printf("Skipping a P_NONE symbol\n");
    if (fread(sym, sizeof(struct symtable), 1, in)!=1)
      return(-1);
  }

  // Get the symbol name
  if (sym->name != NULL) {
    fgetstr(Text, TEXTLEN + 1, in);
    sym->name= strdup(Text);
  }

  // Get any initial values
  if (sym->initlist != NULL) {
    sym->initlist= (int *)malloc(sym->nelems* sizeof(int));
    fread(sym->initlist, sizeof(int), sym->nelems, in);
  }

  // If there are any members, read them in
  if (sym->member != NULL) {
    sym->member= last= NULL;

    while (1) {
      // Create an empty symbol struct
      memb= (struct symtable *)malloc(sizeof(struct symtable));

      // Read the struct in from the in file
      // Stop if no symbols in the file
      if (deserialiseSym(memb, in)== -1) return(0);

      // Attach this to either the head or the last
      if (sym->member==NULL) {
        sym->member= last= memb;
      } else {
	last->next= memb;
        last= memb;
      }

      // Stop if there is no next member
      if (memb->next == NULL) return(0);
    }
  }

  // For now set ctype to NULL
  sym->ctype=NULL;

  return(0);
}

void dumptable(struct symtable *head, int indent);

// Dump a single symbol
void dumpsym(struct symtable *sym, int indent) {
  int i;

  for (i = 0; i < indent; i++)
    printf(" ");
  switch (sym->type & (~0xf)) {
    case P_VOID:
      printf("void ");
      break;
    case P_CHAR:
      printf("char ");
      break;
    case P_INT:
      printf("int ");
      break;
    case P_LONG:
      printf("long ");
      break;
    case P_STRUCT:
      printf("struct ");
      break;
    case P_UNION:
      printf("union ");
      break;
    default:
      printf("unknown type ");
  }

  for (i = 0; i < (sym->type & 0xf); i++)
    printf("*");
  printf("%s", sym->name);

  switch (sym->stype) {
    case S_VARIABLE:
      break;
    case S_FUNCTION:
      printf("()");
      break;
    case S_ARRAY:
      printf("[]");
      break;
    case S_STRUCT:
      printf(": struct");
      break;
    case S_UNION:
      printf(": union");
      break;
    case S_ENUMTYPE:
      printf(": enum");
      break;
    case S_ENUMVAL:
      printf(": enumval");
      break;
    case S_TYPEDEF:
      printf(": typedef");
      break;
    case S_STRLIT:
      printf(": strlit");
      break;
    default:
      printf(" unknown stype");
  }

  printf(" id %d", sym->id);

  switch (sym->class) {
    case V_GLOBAL:
      printf(": global");
      break;
    case V_LOCAL:
      printf(": local offset %d", sym->st_posn);
      break;
    case V_PARAM:
      printf(": param offset %d", sym->st_posn);
      break;
    case V_EXTERN:
      printf(": extern");
      break;
    case V_STATIC:
      printf(": static");
      break;
    case V_MEMBER:
      printf(": member");
      break;
    default:
      printf(": unknown class");
  }

  if (sym->st_hasaddr!=0)
    printf(", hasaddr ");

  switch (sym->stype) {
    case S_VARIABLE:
      printf(", size %d", sym->size);
      break;
    case S_FUNCTION:
      printf(", %d params", sym->nelems);
      break;
    case S_ARRAY:
      printf(", %d elems, size %d", sym->nelems, sym->size);
      break;
  }

  printf(", ctypeid %d, nelems %d st_posn %d\n",
	sym->ctypeid, sym->nelems, sym->st_posn);

  if (sym->initlist != NULL) {
    printf("  initlist: ");
    for (i=0; i< sym->nelems; i++)
      printf("%d ", sym->initlist[i]);
    printf("\n");
  }

  
  if (sym->member != NULL)
    dumptable(sym->member, 4);
}

// Dump one symbol table
void dumptable(struct symtable *head, int indent) {
  struct symtable *sym;

  for (sym = head; sym != NULL; sym = sym->next)
    dumpsym(sym, indent);
}

int main(int argc, char **argv) {
  FILE *in;
  struct symtable sym;

  if (argc !=2) {
    fprintf(stderr, "Usage: %s symbolfile\n", argv[0]); exit(1);
  }

  in= fopen(argv[1], "r");
  if (in==NULL) {
    fprintf(stderr, "Unable to open %s\n", argv[1]); exit(1);
  }

  while (1) {
    if (deserialiseSym(&sym, in)== -1) break;
    dumpsym(&sym, 0);
  }
  exit(0);
  return(0);
}
