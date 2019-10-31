#include "defs.h"
#include "data.h"
#include "decl.h"

// Symbol table functions
// Copyright (c) 2019 Warren Toomey, GPL3

// Determine if the symbol s is in the global symbol table.
// Return its slot position or -1 if not found.
int findglob(char *s) {
  int i;

  for (i = 0; i < Globs; i++) {
    if (*s == *Symtable[i].name && !strcmp(s, Symtable[i].name))
      return (i);
  }
  return (-1);
}

// Get the position of a new global symbol slot, or die
// if we've run out of positions.
static int newglob(void) {
  int p;

  if ((p = Globs++) >= Locls)
    fatal("Too many global symbols");
  return (p);
}

// Determine if the symbol s is in the local symbol table.
// Return its slot position or -1 if not found.
int findlocl(char *s) {
  int i;

  for (i = Locls + 1; i < NSYMBOLS; i++) {
    if (*s == *Symtable[i].name && !strcmp(s, Symtable[i].name))
      return (i);
  }
  return (-1);
}

// Get the position of a new local symbol slot, or die
// if we've run out of positions.
static int newlocl(void) {
  int p;

  if ((p = Locls--) <= Globs)
    fatal("Too many local symbols");
  return (p);
}

// Update a symbol at the given slot number in the symbol table. Set up its:
// + type: char, int etc.
// + structural type: var, function, array etc.
// + size: number of elements
// + endlabel: if this is a function
// + posn: Position information for local symbols
static void updatesym(int slot, char *name, int type, int stype,
		      int class, int endlabel, int size, int posn) {
  if (slot < 0 || slot >= NSYMBOLS)
    fatal("Invalid symbol slot number in updatesym()");
  Symtable[slot].name = strdup(name);
  Symtable[slot].type = type;
  Symtable[slot].stype = stype;
  Symtable[slot].class = class;
  Symtable[slot].endlabel = endlabel;
  Symtable[slot].size = size;
  Symtable[slot].posn = posn;
}

// Add a global symbol to the symbol table. Set up its:
// + type: char, int etc.
// + structural type: var, function, array etc.
// + size: number of elements
// + endlabel: if this is a function
// Return the slot number in the symbol table
int addglob(char *name, int type, int stype, int endlabel, int size) {
  int slot;

  // If this is already in the symbol table, return the existing slot
  if ((slot = findglob(name)) != -1)
    return (slot);

  // Otherwise get a new slot, fill it in and
  // return the slot number
  slot = newglob();
  updatesym(slot, name, type, stype, C_GLOBAL, endlabel, size, 0);
  genglobsym(slot);
  return (slot);
}

// Add a local symbol to the symbol table. Set up its:
// + type: char, int etc.
// + structural type: var, function, array etc.
// + size: number of elements
// + endlabel: if this is a function
// Return the slot number in the symbol table
int addlocl(char *name, int type, int stype, int endlabel, int size) {
  int slot, posn;

  // If this is already in the symbol table, return the existing slot
  if ((slot = findlocl(name)) != -1)
    return (slot);

  // Otherwise get a new symbol slot and a position for this local.
  // Update the symbol table entry and return the slot number
  slot = newlocl();
  posn = gengetlocaloffset(type, 0);	// XXX 0 for now
  updatesym(slot, name, type, stype, C_LOCAL, endlabel, size, posn);
  return (slot);
}

// Determine if the symbol s is in the symbol table.
// Return its slot position or -1 if not found.
int findsymbol(char *s) {
  int slot;

  slot = findlocl(s);
  if (slot == -1)
    slot = findglob(s);
  return (slot);
}
