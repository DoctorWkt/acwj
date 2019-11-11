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
// + structural type: var, function, array etc.
// + size: number of elements, or endlabel: end label for a function
// + posn: Position information for local symbols
// Return a pointer to the new node
struct symtable *newsym(char *name, int type, int stype, int class,
			int size, int posn) {

  // Get a new node
  struct symtable *node = (struct symtable *) malloc(sizeof(struct symtable));
  if (node == NULL)
    fatal("Unable to malloc a symbol table node in newsym");

  // Fill in the values
  node->name = strdup(name);
  node->type = type;
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
struct symtable *addglob(char *name, int type, int stype, int class, int size) {
  struct symtable *sym = newsym(name, type, stype, class, size, 0);
  appendsym(&Globhead, &Globtail, sym);
  return (sym);
}

// Add a symbol to the local symbol list
struct symtable *addlocl(char *name, int type, int stype, int class, int size) {
  struct symtable *sym = newsym(name, type, stype, class, size, 0);
  appendsym(&Loclhead, &Locltail, sym);
  return (sym);
}

// Add a symbol to the parameter list
struct symtable *addparm(char *name, int type, int stype, int class, int size) {
  struct symtable *sym = newsym(name, type, stype, class, size, 0);
  appendsym(&Parmhead, &Parmtail, sym);
  return (sym);
}

// Search for a symbol in a specific list.
// Return a pointer to the found node or NULL if not found.
static struct symtable *findsyminlist(char *s, struct symtable *list) {
  for (; list != NULL; list = list->next)
    if ((list->name != NULL) && !strcmp(s, list->name))
      return (list);
  return (NULL);
}

// Determine if the symbol s is in the global symbol table.
// Return a pointer to the found node or NULL if not found.
struct symtable *findglob(char *s) {
  return (findsyminlist(s, Globhead));
}

// Determine if the symbol s is in the local symbol table.
// Return a pointer to the found node or NULL if not found.
struct symtable *findlocl(char *s) {
  struct symtable *node;

  // Look for a parameter if we are in a function's body
  if (Functionid) {
    node = findsyminlist(s, Functionid->member);
    if (node)
      return (node);
  }
  return (findsyminlist(s, Loclhead));
}

// Determine if the symbol s is in the symbol table.
// Return a pointer to the found node or NULL if not found.
struct symtable *findsymbol(char *s) {
  struct symtable *node;

  // Look for a parameter if we are in a function's body
  if (Functionid) {
    node = findsyminlist(s, Functionid->member);
    if (node)
      return (node);
  }
  // Otherwise, try the local and global symbol lists
  node = findsyminlist(s, Loclhead);
  if (node)
    return (node);
  return (findsyminlist(s, Globhead));
}

// Find a composite type.
// Return a pointer to the found node or NULL if not found.
struct symtable *findcomposite(char *s) {
  return (findsyminlist(s, Comphead));
}

// Reset the contents of the symbol table
void clear_symtable(void) {
  Globhead = Globtail = NULL;
  Loclhead = Locltail = NULL;
  Parmhead = Parmtail = NULL;
  Comphead = Comptail = NULL;
}

// Clear all the entries in the local symbol table
void freeloclsyms(void) {
  Loclhead = Locltail = NULL;
  Parmhead = Parmtail = NULL;
  Functionid = NULL;
}
