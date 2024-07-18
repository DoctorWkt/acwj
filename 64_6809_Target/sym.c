#include "defs.h"
#include "data.h"
#include "misc.h"
#include "tree.h"
#include "types.h"
#include "sym.h"

#undef MEMBDEBUG
#undef DEBUG

// Symbol table functions
// Copyright (c) 2024 Warren Toomey, GPL3

// We have two in-memory symbol tables. One is for
// types (structs, unions, enums, typedefs); the
// other is for variables and functions. These
// cache symbols from the symbol file which have
// been recently used.
// There is also a temporary list which we build
// before attaching to a symbol's member field.
// This is used for structs, unions and enums.
// It also holds the parameters and locals of
// the function we are currently parsing.
//
// Symhead needs to be visible for cgen.c

struct symtable *Symhead = NULL;
static struct symtable *Symtail = NULL;
static struct symtable *Typehead = NULL;
static struct symtable *Typetail = NULL;
static struct symtable *Membhead = NULL;
static struct symtable *Membtail = NULL;

#ifdef DEBUG
static void dumptable(struct symtable *head, int indent);

// List of structural type strings
char *Sstring[] = {
  "variable", "function", "array", "enumval", "strlit",
  "struct", "union", "enumtype", "typedef", "notype"
};

// Dump a single symbol
static void dumpsym(struct symtable *sym, int indent) {
  int i;

  for (i = 0; i < indent; i++)
    printf(" ");
  switch (sym->type & (~0xf)) {
  case P_VOID: printf("void "); break;
  case P_CHAR: printf("char "); break;
  case P_INT:  printf("int "); break;
  case P_LONG: printf("long "); break;
  case P_STRUCT:
    printf("struct ");
    if (sym->ctype && sym->ctype->name)
      printf("%s ", sym->ctype->name);
    break;
  case P_UNION:
    printf("union ");
    if (sym->ctype && sym->ctype->name)
      printf("%s ", sym->ctype->name);
    break;
  default:
    printf("unknown type ");
  }

  for (i = 0; i < (sym->type & 0xf); i++) printf("*");
  printf("%s", sym->name);

  switch (sym->stype) {
  case S_VARIABLE:
    break;
  case S_FUNCTION: printf("()"); break;
  case S_ARRAY:    printf("[]"); break;
  case S_STRUCT:   printf(": struct"); break;
  case S_UNION:    printf(": union"); break;
  case S_ENUMTYPE: printf(": enum"); break;
  case S_ENUMVAL:  printf(": enumval"); break;
  case S_TYPEDEF:  printf(": typedef"); break;
  case S_STRLIT:   printf(": strlit"); break;
  default:	   printf(" unknown stype");
  }

  printf(" id %d", sym->id);

  switch (sym->class) {
  case V_GLOBAL: printf(": global"); break;
  case V_LOCAL:  printf(": local offset %d", sym->st_posn); break;
  case V_PARAM:  printf(": param offset %d", sym->st_posn); break;
  case V_EXTERN: printf(": extern"); break;
  case V_STATIC: printf(": static"); break;
  case V_MEMBER: printf(": member"); break;
  default: 	 printf(": unknown class");
  }

  if (sym->st_hasaddr != 0) printf(", hasaddr ");

  switch (sym->stype) {
  case S_VARIABLE: printf(", size %d", sym->size); break;
  case S_FUNCTION: printf(", %d params", sym->nelems); break;
  case S_ARRAY:    printf(", %d elems, size %d", sym->nelems, sym->size); break;
  }

  printf(", ctypeid %d, nelems %d st_posn %d\n",
	 sym->ctypeid, sym->nelems, sym->st_posn);

  if (sym->member != NULL) dumptable(sym->member, 4);
}

// Dump one symbol table
static void dumptable(struct symtable *head, int indent) {
  struct symtable *sym;

  for (sym = head; sym != NULL; sym = sym->next) dumpsym(sym, indent);
}

void dumpSymlists(void) {
  fprintf(stderr, "Typelist\n");
  fprintf(stderr, "--------\n");
  dumptable(Typehead, 0);
  fprintf(stderr, "\nSymlist\n");
  fprintf(stderr, "-------\n");
  dumptable(Symhead, 0);
  fprintf(stderr, "\nFunctionid\n");
  fprintf(stderr, "----------\n");
  dumptable(Functionid, 0);
  fprintf(stderr, "\nMemblist\n");
  fprintf(stderr, "--------\n");
  dumptable(Membhead, 0);
}
#endif

// Append a node to the singly-linked list pointed to by head or tail
static void appendSym(struct symtable **head, struct symtable **tail,
		      struct symtable *node) {

  // Check for valid pointers
  if (head == NULL || tail == NULL || node == NULL)
    fatal("Either head, tail or node is NULL in appendSym");

  // Append to the list
  if (*tail) {
    (*tail)->next = node; *tail = node;
  } else
    *head = *tail = node;
  node->next = NULL;
}

// The last name we loaded from the symbol file
static char SymText[TEXTLEN + 1];

#ifdef WRITESYMS
// Unique id for each symbol. We need this when serialising
// so that the composite type of a variable can be found.
static int Symid = 1;

// For symbols which are not locals or parameters,
// we point this at the symbol once created. This
// allows us to e.g. add initial values or members.
// When we come to make another non-local/param,
// this gets flushed to disk.

struct symtable *thisSym = NULL;

// When we are serialising symbols to the symbol table file,
// track the one with the highest id. After each flushing
// of the in-memory lists, record the highest id into skipSymid.
// Then, on the next flush, don't write out symbols at or below
// the skipSymid.
static int highestSymid = 0;
static int skipSymid = 0;

// Serialise one symbol to the symbol table file
static void serialiseSym(struct symtable *sym) {
  struct symtable *memb;

  if (sym->id > highestSymid) highestSymid = sym->id;

  if (sym->id <= skipSymid) {
#ifdef DEBUG
    fprintf(stderr, "NOT Writing %s %s id %d to disk\n",
	    Sstring[sym->stype], sym->name, sym->id);
#endif
    return;
  }

  // Output the symbol struct and the name
  // once we are at the end of the file
  fseek(Symfile, 0, SEEK_END);
#ifdef DEBUG
  fprintf(stderr, "Writing %s %s id %d to disk offset %ld\n",
	  Sstring[sym->stype], sym->name, sym->id, ftell(Symfile));
#endif
  fwrite(sym, sizeof(struct symtable), 1, Symfile);
  if (sym->name != NULL) {
    fputs(sym->name, Symfile); fputc(0, Symfile);
  }
  // Output the initial values, if any
  if (sym->initlist != NULL)
    fwrite(sym->initlist, sizeof(int), sym->nelems, Symfile);

  // Output the member symbols
#ifdef DEBUG
  if (sym->member != NULL) {
    fprintf(stderr, "%s has members\n", sym->name);
  }
#endif

  for (memb = sym->member; memb != NULL; memb = memb->next)
    serialiseSym(memb);
}

// Create a symbol table node. Set up the node's:
// + type: char, int etc.
// + ctype: composite type pointer for struct/union
// + structural type: var, function, array etc.
// + size: number of elements, or endlabel: end label for a function
// + posn: Position information for local symbols
// Return a pointer to the new node.
static struct symtable *newsym(char *name, int type, struct symtable *ctype,
			       int stype, int class, int nelems, int posn) {

  // Get a new node
  struct symtable *node = (struct symtable *) malloc(sizeof(struct symtable));
  if (node == NULL)
    fatal("Unable to malloc a symbol table node in newsym");

  // Fill in the values
  node->id = Symid++;
#ifdef DEBUG
  fprintf(stderr, "Newsym %s %s id %d\n", Sstring[stype], name, node->id);
#endif
  if (name == NULL) node->name = NULL;
  else node->name = strdup(name);
  node->type = type;
  node->ctype = ctype;
  if (ctype != NULL) node->ctypeid = ctype->id;
  else node->ctypeid = 0;
  node->stype = stype;
  node->class = class;
  node->nelems = nelems;
  node->st_hasaddr = 0;

  // For pointers and integer types, set the size
  // of the symbol. structs and union declarations
  // manually set this up themselves.
  if (ptrtype(type) || inttype(type))
    node->size = nelems * typesize(type, ctype);

  node->st_posn = posn;
  node->next = NULL;
  node->member = NULL;
  node->initlist = NULL;
  return (node);
}

// Add a new type to the list of types. Return a pointer to the symbol.
struct symtable *addtype(char *name, int type, struct symtable *ctype,
			 int stype, int class, int nelems, int posn) {
  struct symtable *sym;
  thisSym = sym = newsym(name, type, ctype, stype, class, nelems, posn);

#ifdef DEBUG
  fprintf(stderr, "Added %s %s to Typelist\n",
	  Sstring[sym->stype], sym->name);
#endif
  appendSym(&Typehead, &Typetail, sym);
  Membhead = Membtail = NULL;
  return (sym);
}

// Add a new symbol to the global table. Return a pointer to the symbol.
struct symtable *addglob(char *name, int type, struct symtable *ctype,
			 int stype, int class, int nelems, int posn) {
  struct symtable *sym;
  thisSym = sym = newsym(name, type, ctype, stype, class, nelems, posn);

  // For structs and unions, copy the size from the type node
  if (type == P_STRUCT || type == P_UNION) sym->size = ctype->size;

#ifdef DEBUG
  fprintf(stderr, "Added %s %s to Symlist\n", Sstring[sym->stype], sym->name);
#endif
  appendSym(&Symhead, &Symtail, sym);
  Membhead = Membtail = NULL;
  return (sym);
}

// Add a symbol to the member list in thisSym
struct symtable *addmemb(char *name, int type, struct symtable *ctype,
			 int class, int stype, int nelems) {
  struct symtable *sym = newsym(name, type, ctype, stype, class, nelems, 0);

  // For structs and unions, copy the size from the type node
  if (type == P_STRUCT || type == P_UNION) sym->size = ctype->size;

  // Add this to the member list and link into thisSym if needed
  appendSym(&Membhead, &Membtail, sym);
#ifdef DEBUG
  fprintf(stderr, "Added %s %s to Memblist\n",
	  Sstring[sym->stype], sym->name);
#endif
  if (thisSym->member == NULL) {
    thisSym->member = Membhead;
#ifdef DEBUG
    fprintf(stderr, "Added %s to start of %s\n", name, thisSym->name);
#endif
  }
  return (sym);
}

// Flush the contents of the in-memory symbol tables
// to the file.

void flushSymtable() {
  struct symtable *this;

  // Write out types
  for (this = Typehead; this != NULL; this = this->next) {
    serialiseSym(this);
  }

  // Write out variables and functions.
  // Skip invalid symbols
  for (this = Symhead; this != NULL; this = this->next) {
    serialiseSym(this);
  }

  skipSymid = highestSymid;
  freeSymtable();
}
#endif // WRITESYMS

// When reading in globals that have members (structs, unions,
// functions), we stop once we hit a non-member. We need to
// record that symbol's offset so we can fseek() back to that.
// Otherwise we would not be able to load it if it also was a global.
static long lastSymOffset;

// We need a linked list when loadSym() loads in members of a symbol
// from the disk. We can't use Membhead/tail as this might be in use
// when parsing the body of a function. So we keep a private list.
static struct symtable *Mhead, *Mtail;

// Given a pointer to a symtable node, read in the next entry
// in the on-disk symbol table. Do this always if loadit is true.
// Only read one node if recurse is zero.
// If loadit is false, load the data and return true if the symbol
// a) matches the given name and stype or b) matches the id.
// Return -1 when there is nothing left to read.
static int loadSym(struct symtable *sym, char *name,
		   int stype, int id, int loadit, int recurse) {
  struct symtable *memb;

#ifdef DEBUGTOOMUCH
if (name!=NULL)
  fprintf(stderr, "loadSym: name %s stype %d loadit %d recurse %d\n",
		name, stype, loadit, recurse);
else
  fprintf(stderr, "loadSym: id %d stype %d loadit %d recurse %d\n",
		id, stype, loadit, recurse);
#endif

  // Read in the next node. Get a copy of the offset beforehand
  lastSymOffset = ftell(Symfile);
  if (fread(sym, sizeof(struct symtable), 1, Symfile) != 1) return (-1);

  // Get the symbol name into a separate buffer for now
  if (sym->name != NULL) {
    fgetstr(SymText, TEXTLEN + 1, Symfile);
  }

#ifdef DEBUG
  if (sym->name != NULL)
    fprintf(stderr, "symoff %ld name %s stype %d\n",
		lastSymOffset, SymText, sym->stype);
  else
    fprintf(stderr, "symoff %ld id %d\n", lastSymOffset, sym->id);
#endif

  // If loadit is off, see if the ids match. Or,
  // see if the names are a match and the stype matches.
  // For the latter, if NOTATYPE match anything which isn't
  // a type and which isn't a member, local or param: we are
  // trying to find a variable, enumval or function. findlocl()
  // will find it if it's a local or parameter. We only get
  // here when we are trying to find a global variable,
  // enumval or function.
  if (loadit == 0) {
    if (id != 0 && sym->id == id) loadit = 1;
    if (name != NULL && !strcmp(name, SymText)) {
      if (stype == S_NOTATYPE && sym->stype < S_STRUCT
	  		      && sym->class < V_LOCAL) loadit = 1;
      if (stype >= S_STRUCT && stype == sym->stype) loadit = 1;
    }
  }

  // Yes, we need to load the rest of the symbol
  if (loadit) {

    // Copy the name over.
    sym->name = strdup(SymText);
    if (sym->name == NULL) fatal("Unable to malloc name in loadSym()");

#ifdef DEBUG
    if (sym->name == NULL) {
      fprintf(stderr, "loadSym found %s NONAME id %d loadit %d\n",
	      Sstring[sym->stype], sym->id, loadit);
    } else {
      fprintf(stderr, "loadSym found %s %s id %d loadit %d\n",
	      Sstring[sym->stype], sym->name, sym->id, loadit);
    }
#endif

    // Get the initialisation list.
    if (sym->initlist != NULL) {
      sym->initlist = (int *) malloc(sym->nelems * sizeof(int));
      if (sym->initlist == NULL)
	fatal("Unable to malloc initlist in loadSym()");
      fread(sym->initlist, sizeof(int), sym->nelems, Symfile);
    }

    // Stop now if we must not recursively load more nodes
    if (!recurse) {
#ifdef DEBUG
      fprintf(stderr, "loadSym found it - no recursion\n");
#endif
      return (1);
    }

    // For structs, unions and functions load and add
    // the members (or params/locals) to the member list
    if (sym->stype == S_STRUCT || sym->stype == S_UNION
			       || sym->stype == S_FUNCTION) {
      Mhead = Mtail = NULL;

      while (1) {
#ifdef DEBUG
fprintf(stderr, "loadSym: about to try loading members\n");
#endif
	memb = (struct symtable *) malloc(sizeof(struct symtable));
	if (memb == NULL)
	  fatal("Unable to malloc member in loadSym()");
#ifdef MEMBDEBUG
fprintf(stderr, "%p allocated\n", memb);
#endif
	// Get the next symbol. Stop when there are no symbols
	// or when the symbol isn't a member, enumval, param or local
	if (loadSym(memb, NULL, 0, 0, 1, 0) != 1)
	  break;
	if (memb->class != V_LOCAL && memb->class != V_PARAM &&
	    memb->class != V_MEMBER)
	  break;
#ifdef DEBUG
fprintf(stderr, "loadSym: appending %s to member list\n", memb->name);
#endif
	appendSym(&Mhead, &Mtail, memb);
      }

      // We found a non-member symbol. Seek back
      // to where it was and free the unused struct.
      // Attach the member list to the original symbol.
      fseek(Symfile, lastSymOffset, SEEK_SET);
#ifdef DEBUG
fprintf(stderr, "Seeked to lastSymOffset %ld as non-member id %d\n",
				lastSymOffset, memb->id);
#endif
#ifdef MEMBDEBUG
fprintf(stderr, "%p freed, unused memb\n", memb);
#endif
      free(memb);
      sym->member = Mhead;
      Mhead = Mtail = NULL;
    }
    return (1);
  } else {
    // No match and loadit was 0. Skip over any initialisation list.
    if (sym->initlist != NULL)
      fseek(Symfile, sizeof(int) * sym->nelems, SEEK_CUR);
  }
  return (0);
}

// Given a name or an id, search the symbol table file for the next
// symbol that matches. Fill in the node and return true on a match.
// Otherwise, return false.
static int findSyminfile(struct symtable *sym, char *name, int id, int stype) {
  int res;

#ifdef DEBUG
if (name!=NULL)
  fprintf(stderr, "findSyminfile: searching name %s stype %d\n", name, stype);
else
  fprintf(stderr, "findSyminfile: search id %d\n", id);
#endif

  // Loop over the file starting at the beginning
  fseek(Symfile, 0, SEEK_SET);
  while (1) {
    // Does the next symbol match? Yes, return it
    res = loadSym(sym, name, stype, id, 0, 1);
    if (res == 1) return (1);
    if (res == -1) break;
  }
#ifdef DEBUG
  fprintf(stderr, "findSyminfile: not found\n");
#endif
  return (0);
}

// Determine if the symbol name, or id if not zero, is a local
// or parameter. Return a pointer to the found node or NULL if not found.
struct symtable *findlocl(char *name, int id) {
  struct symtable *this;

  // We must be in a function
  if (Functionid == NULL) return (NULL);

#ifdef DEBUG
if (id!=0) fprintf(stderr, "findlocl() searching for id %d\n", id);
if (name!=NULL) fprintf(stderr, "findlocl() searching for name %s\n", name);
#endif

  for (this = Functionid->member; this != NULL; this = this->next) {
    if (id && this->id == id) return (this);
    if (name && !strcmp(this->name, name)) return (this);
  }

  return (NULL);
}

// Given a name and a stype, search for a matching symbol.
// Or, if the id is non-zero, search for the symbol with that id.
// Bring the symbol in to one of the in-memory lists if necessary.
// Return a pointer to the found node or NULL if not found.
struct symtable *findSymbol(char *name, int stype, int id) {
  struct symtable *this;
  struct symtable *sym;
  int notatype;

  // Set a flag if we are not looking for a type
  notatype = (stype == S_NOTATYPE || stype == S_ENUMVAL);

#ifdef DEBUG
  if (id != 0)
    fprintf(stderr, "Searching for symbol id %d in memory\n", id);
  else
    fprintf(stderr, "Searching for symbol %s %s in memory\n",
	    Sstring[stype], name);
#endif

  // If it's not a type, see if
  // it's a local or parameter
  if (id || notatype) {
    this = findlocl(name, id);
    if (this != NULL) return (this);

#ifdef DEBUG
fprintf(stderr, "Not in local, try the global Symlist\n");
#endif

    // Not a local, so search the global symbol list.
    for (this = Symhead; this != NULL; this = this->next) {
      if (id && this->id == id) return (this);
      if (name && !strcmp(this->name, name)) return (this);
    }
  }

#ifdef DEBUG
fprintf(stderr, "Not in , try the global Typelist\n");
#endif

  // We have an id or it is a type,
  // Search the global type list.
  // Sorry for the double negative :-)
  if (id || !notatype) {
    for (this = Typehead; this != NULL; this = this->next) {
      if (id && this->id == id) return (this);
      if (name && !strcmp(this->name, name) && this->stype == stype)
	return (this);
    }
  }

#ifdef DEBUG
  fprintf(stderr, "  not in memory, try the file\n");
#endif

  // Not in memory. Try the on-disk symbol table
  sym = (struct symtable *) malloc(sizeof(struct symtable));
  if (sym == NULL) {
    fatal("Unable to malloc sym in findSyminlist()");
  }

  // If we found a match in the file
  if (findSyminfile(sym, name, id, stype)) {
    // Add it to one of the in-memory lists and return it
    if (sym->stype < S_STRUCT)
      appendSym(&Symhead, &Symtail, sym);
    else
      appendSym(&Typehead, &Typetail, sym);

    // If the symbol points at a composite type, find and link it
    if (sym->ctype != NULL) {
#ifdef DEBUG
      fprintf(stderr, "About to findSymid on id %d for %s\n", sym->ctypeid,
	      sym->name);
#endif
      sym->ctype = findSymbol(NULL, 0, sym->ctypeid);
    }

    // If any member symbols point at a composite type, ditto
    for (this = sym->member; this != NULL; this = this->next)
      if (this->ctype != NULL) {
#ifdef DEBUG
	fprintf(stderr, "About to member findSymid on id %d for %s\n",
		this->ctypeid, this->name);
#endif
	this->ctype = findSymbol(NULL, 0, this->ctypeid);
      }
    return (sym);
  }
  free(sym);
  return (NULL);
}

// Find a member in the member list. Return a pointer
// to the found node or NULL if not found.
struct symtable *findmember(char *s) {
  struct symtable *node;

  for (node = Membhead; node != NULL; node = node->next)
    if (!strcmp(s, node->name)) return (node);

  return (NULL);
}

// Find a node in the struct list
// Return a pointer to the found node or NULL if not found.
struct symtable *findstruct(char *s) {
  return (findSymbol(s, S_STRUCT, 0));
}

// Find a node in the union list
// Return a pointer to the found node or NULL if not found.
struct symtable *findunion(char *s) {
  return (findSymbol(s, S_UNION, 0));
}

// Find an enum type in the enum list
// Return a pointer to the found node or NULL if not found.
struct symtable *findenumtype(char *s) {
  return (findSymbol(s, S_ENUMTYPE, 0));
}

// Find an enum value in the enum list
// Return a pointer to the found node or NULL if not found.
struct symtable *findenumval(char *s) {
  return (findSymbol(s, S_ENUMVAL, 0));
}

// Find a type in the tyedef list
// Return a pointer to the found node or NULL if not found.
struct symtable *findtypedef(char *s) {
  return (findSymbol(s, S_TYPEDEF, 0));
}

// Free a symbol's memory, returning the symbol's next pointer
struct symtable *freeSym(struct symtable *sym) {
  struct symtable *next, *memb;

  if (sym == NULL) return (NULL);
  next = sym->next;
#ifdef MEMBDEBUG
  fprintf(stderr, "%p freeing\n", sym);
#endif
#ifdef DEBUG
  fprintf(stderr, "Freeing %s %s\n", Sstring[sym->stype], sym->name);
#endif

  // Free any members
  for (memb = sym->member; memb != NULL;)
    memb = freeSym(memb);

  // Free the initlist and the name
  if (sym->initlist != NULL)
    free(sym->initlist);
  if (sym->name != NULL)
    free(sym->name);
  free(sym);
  return (next);
}

// Free the contents of the in-memory symbol tables
void freeSymtable() {
  struct symtable *this;

  for (this = Symhead; this != NULL;)
    this = freeSym(this);
  for (this = Typehead; this != NULL;)
    this = freeSym(this);
  Symhead = Symtail = Typehead = Typetail = NULL;
  Membhead = Membtail = Functionid = NULL;
}

// Loop over the symbol table file.
// Load in all the types and
// global/static variables.
void loadGlobals(void) {
  struct symtable *sym;
  int i;

  // Start at the file's beginning. Load all symbols
  fseek(Symfile, 0, SEEK_SET);
  while (1) {
    // Load the next symbol + members + initlist
    sym = (struct symtable *) malloc(sizeof(struct symtable));
    if (sym == NULL)
      fatal("Unable to malloc in allocateGlobals()");
    i = loadSym(sym, NULL, 0, 0, 1, 1);
    if (i == -1) {
      free(sym); break;
    }

    // Add any type to the type list
    if (sym->stype >= S_STRUCT) {
      appendSym(&Typehead, &Typetail, sym); continue;
    }

    // Add any global/static variable/array/strlit to the sym list
    if ((sym->class == V_GLOBAL || sym->class == V_STATIC) &&
	(sym->stype == S_VARIABLE || sym->stype == S_ARRAY ||
	 sym->stype == S_STRLIT)) {
      appendSym(&Symhead, &Symtail, sym);

      // If the symbol points at a composite type, find and link it
      if (sym->ctype != NULL) {
	sym->ctype = findSymbol(NULL, 0, sym->ctypeid);
      }
      continue;
    }

    // Didn't add it to any list
    freeSym(sym);
  }
}
