// sym.c
struct symtable *addtype(char *name, int type, struct symtable *ctype,
				int stype, int class, int nelems, int posn);
struct symtable *addglob(char *name, int type, struct symtable *ctype,
				int stype, int class, int nelems, int posn);
struct symtable *addmemb(char *name, int type, struct symtable *ctype,
				          int class, int stype, int nelems);
struct symtable *findlocl(char *name, int id);
struct symtable *findSymbol(char *name, int stype, int id);
struct symtable *findmember(char *s);
struct symtable *findstruct(char *s);
struct symtable *findunion(char *s);
struct symtable *findenumtype(char *s);
struct symtable *findenumval(char *s);
struct symtable *findtypedef(char *s);
void loadGlobals(void);
struct symtable *freeSym(struct symtable *sym);
void freeSymtable(void);
void flushSymtable(void);
void dumpSymlists(void);

extern struct symtable *Symhead;
