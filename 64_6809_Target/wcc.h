#define AOUT	"a.out"
#define TMPDIR 	"/tmp"

/////////////////
// QBE SECTION //
/////////////////

// List of phase command strings
char *qbephasecmd[]= {
  "cpp",				// C pre-processor
  BINDIR "/cscan",			// Tokeniser
  BINDIR "/cparseqbe",			// Parser
  BINDIR "/cgenqbe",			// Code generator
  "qbe",				// QBE to assembler
  "as",					// Assembler
  "cc"					// Linker
};

// List of C preprocessor flags
char *qbecppflags[]= {
  "-nostdinc",
  "-isystem",
  INCQBEDIR,
  NULL
};

// List of object files that
// must precede any compiled ones
// e.g. crt0.o files
char *qbepreobjs[]= {
  NULL
};

// List of object files and/or
// libraries that must come
// after any compiled ones
char *qbepostobjs[]= {
  NULL
};

//////////////////
// 6809 SECTION //
//////////////////

// List of phase command strings
char *phasecmd6809[]= {
  "cpp",				// C pre-processor
  BINDIR "/cscan",			// Tokeniser
  BINDIR "/cparse6809",			// Parser
  BINDIR "/cgen6809",			// Code generator
  BINDIR "/cpeep",			// Peephole optmiser
  "as6809",				// Assembler
  "ld6809"				// Linker
};

// List of C preprocessor flags
char *cppflags6809[]= {
  "-nostdinc",
  "-isystem",
  INC6809DIR,
  NULL
};

// List of object files that
// must precede any compiled ones
// e.g. crt0.o files
char *preobjs6809[]= {
  LIB6809DIR "/crt0.o",
  NULL
};

// List of object files and/or
// libraries that must come
// after any compiled ones
char *postobjs6809[]= {
  LIB6809DIR "/libc.a",
  LIB6809DIR "/lib6809.a",
  NULL
};
