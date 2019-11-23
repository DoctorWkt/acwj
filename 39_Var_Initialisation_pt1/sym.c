#include "defs.h"
#include "data.h"
#include "decl.h"

// Symbol table functions
// Copyright (c) 2019 Warren Toomey, GPL3

// Append a node to the singly-linked list pointed to by head or tail
void appendsym(struct symtable **head, struct symtable **tail,
	       struct symtable *node) {

  // Check for valid pointers
  if (head == NULL || tail == NULL || node == NULL)
    fatal("Either head, tail or node is NULL in appendsym");

  // Append to the list
  if (*tail) {
    (*tail)->next = node;
    *tail = node;
  } else
    *head = *tail = node;
  node->next = NULL;
}

// Create a symbol node to be added to a symbol table list.
// Set up the node's:
// + type: char, int etc.
// + ctype: composite type pointer for struct/union
// + structural type: var, function, array etc.
// + size: number of elements, or endlabel: end label for a function
// + posn: Position information for local symbols
// Return a pointer to the new node
struct symtable *newsym(char *name, int type, struct symtable *ctype,
			int stype, int class, int size, int posn) {

  // Get a new node
  struct symtable *node = (struct symtable *) malloc(sizeof(struct symtable));
  if (node == NULL)
    fatal("Unable to malloc a symbol table node in newsym");

  // Fill in the values
  node->name = strdup(name);
  node->type = type;
  node->ctype = ctype;
  node->stype = stype;
  node->class = class;
  node->size = size;
  node->posn = posn;
  node->next = NULL;
  node->member = NULL;

  // Generate any global space
  if (class == C_GLOBAL)
    genglobsym(node);
  return (node);
}

// Add a symbol to the global symbol list
struct symtable *addglob(char *name, int type, struct symtable *ctype,
			 int stype, int class, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, class, size, 0);
  appendsym(&Globhead, &Globtail, sym);
  return (sym);
}

// Add a symbol to the local symbol list
struct symtable *addlocl(char *name, int type, struct symtable *ctype,
			 int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_LOCAL, size, 0);
  appendsym(&Loclhead, &Locltail, sym);
  return (sym);
}

// Add a symbol to the parameter list
struct symtable *addparm(char *name, int type, struct symtable *ctype,
		 	 int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_PARAM, size, 0);
  appendsym(&Parmhead, &Parmtail, sym);
  return (sym);
}

// Add a symbol to the temporary member list
struct symtable *addmemb(char *name, int type, struct symtable *ctype,
			 int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_MEMBER, size, 0);
  appendsym(&Membhead, &Membtail, sym);
  return (sym);
}

// Add a struct to the struct list
struct symtable *addstruct(char *name, int type, struct symtable *ctype,
			   int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_STRUCT, size, 0);
  appendsym(&Structhead, &Structtail, sym);
  return (sym);
}

// Add a struct to the union list
struct symtable *addunion(char *name, int type, struct symtable *ctype,
			   int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_UNION, size, 0);
  appendsym(&Unionhead, &Uniontail, sym);
  return (sym);
}

// Add an enum type or value to the enum list.
// Class is C_ENUMTYPE or C_ENUMVAL.
// Use posn to store the int value.
struct symtable *addenum(char *name, int class, int value) {
  struct symtable *sym = newsym(name, P_INT, NULL, 0, class, 0, value);
  appendsym(&Enumhead, &Enumtail, sym);
  return (sym);
}

// Add a typedef to the typedef list
struct symtable *addtypedef(char *name, int type, struct symtable *ctype,
			   int stype, int size) {
  struct symtable *sym = newsym(name, type, ctype, stype, C_TYPEDEF, size, 0);
  appendsym(&Typehead, &Typetail, sym);
  return (sym);
}

// Search for a symbol in a specific list.
// Return a pointer to the found node or NULL if not found.
// If class is not zero, also match on the given class
static struct symtable *findsyminlist(char *s, struct symtable *list, int class) {
  for (; list != NULL; list = list->next)
    if ((list->name != NULL) && !strcmp(s, list->name))
      if (class==0 || class== list->class)
        return (list);
  return (NULL);
}

// Determine if the symbol s is in the global symbol table.
// Return a pointer to the found node or NULL if not found.
struct symtable *findglob(char *s) {
  return (findsyminlist(s, Globhead, 0));
}

// Determine if the symbol s is in the local symbol table.
// Return a pointer to the found node or NULL if not found.
struct symtable *findlocl(char *s) {
  struct symtable *node;

  // Look for a parameter if we are in a function's body
  if (Functionid) {
    node = findsyminlist(s, Functionid->member, 0);
    if (node)
      return (node);
  }
  return (findsyminlist(s, Loclhead, 0));
}

// Determine if the symbol s is in the symbol table.
// Return a pointer to the found node or NULL if not found.
struct symtable *findsymbol(char *s) {
  struct symtable *node;

  // Look for a parameter if we are in a function's body
  if (Functionid) {
    node = findsyminlist(s, Functionid->member, 0);
    if (node)
      return (node);
  }
  // Otherwise, try the local and global symbol lists
  node = findsyminlist(s, Loclhead, 0);
  if (node)
    return (node);
  return (findsyminlist(s, Globhead, 0));
}

// Find a member in the member list
// Return a pointer to the found node or NULL if not found.
struct symtable *findmember(char *s) {
  return (findsyminlist(s, Membhead, 0));
}

// Find a struct in the struct list
// Return a pointer to the found node or NULL if not found.
struct symtable *findstruct(char *s) {
  return (findsyminlist(s, Structhead, 0));
}

// Find a struct in the union list
// Return a pointer to the found node or NULL if not found.
struct symtable *findunion(char *s) {
  return (findsyminlist(s, Unionhead, 0));
}

// Find an enum type in the enum list
// Return a pointer to the found node or NULL if not found.
struct symtable *findenumtype(char *s) {
  return (findsyminlist(s, Enumhead, C_ENUMTYPE));
}

// Find an enum value in the enum list
// Return a pointer to the found node or NULL if not found.
struct symtable *findenumval(char *s) {
  return (findsyminlist(s, Enumhead, C_ENUMVAL));
}

// Find a type in the tyedef list
// Return a pointer to the found node or NULL if not found.
struct symtable *findtypedef(char *s) {
  return (findsyminlist(s, Typehead, 0));
}

// Reset the contents of the symbol table
void clear_symtable(void) {
  Globhead =   Globtail  =  NULL;
  Loclhead =   Locltail  =  NULL;
  Parmhead =   Parmtail  =  NULL;
  Membhead =   Membtail  =  NULL;
  Structhead = Structtail = NULL;
  Unionhead =  Uniontail =  NULL;
  Enumhead =   Enumtail =   NULL;
  Typehead =   Typetail =   NULL;
}

// Clear all the entries in the local symbol table
void freeloclsyms(void) {
  Loclhead = Locltail = NULL;
  Parmhead = Parmtail = NULL;
  Functionid = NULL;
}
