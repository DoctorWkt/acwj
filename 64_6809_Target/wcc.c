#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "dirs.h"
#include "wcc.h"

// Compiler setup and top-level execution
// Copyright (c) 2024 Warren Toomey, GPL3

// List of phases
#define CPP_PHASE	0
#define TOK_PHASE	1
#define PARSE_PHASE	2
#define GEN_PHASE	3
#define QBEPEEP_PHASE	4	// Either run QBE or the peephole optimiser
#define ASM_PHASE	5
#define LINK_PHASE	6

// A struct to keep a linked list of filenames
struct filelist {
  char *name;
  struct filelist *next;
};

#define CPU_QBE		1
#define CPU_6809	2

// Global variables
int cpu = CPU_QBE;		// What CPU/platform we are targetting
int last_phase = LINK_PHASE;	// Which is the last phase
int verbose = 0;		// Print out the phase details?
int keep_tempfiles = 0;		// Keep temporary files?
char *outname = NULL;		// Output filename, if any
char *initname;			// File name given to us

				// List of commands and object files
char **phasecmd;
char **cppflags;
char **preobjs;
char **postobjs;

				// And -D preprocessor words get added here
#define MAXCPPEXTRA 20
char *cppextra[20];
int cppxindex=0;

				// Lists of temp and object files
struct filelist *Tmphead, *Tmptail;
struct filelist *Objhead, *Objtail;

#define MAXCMDARGS 500
char *cmdarg[MAXCMDARGS];	// List of arguments to a command
int cmdcount = 0;		// Number of command arguments

// Alter the last letter of the initial filename
char *alter_suffix(char ch) {
  char *str = strdup(initname);
  char *cptr = str + strlen(str) - 1;
  *cptr = ch;
  return (str);
}

// Add a name to the list of temporary files
void addtmpname(char *name) {
  struct filelist *this;
  this = (struct filelist *) malloc(sizeof(struct filelist));
  this->name = name;
  this->next = NULL;
  if (Tmphead == NULL) Tmphead = Tmptail = this;
  else { Tmptail->next = this; Tmptail = this; }
}

// Remove temporary files and exit
void Exit(int val) {
  struct filelist *this;

  if (keep_tempfiles == 0)
    for (this = Tmphead; this != NULL; this = this->next)
      unlink(this->name);
  exit(val);
}

// Add a name to the list of object files
void addobjname(char *name) {
  struct filelist *this;
  this = (struct filelist *) malloc(sizeof(struct filelist));
  this->name = name;
  this->next = NULL;
  if (Objhead == NULL) Objhead = Objtail = this;
  else { Objtail->next = this; Objtail = this; }
}

// Clear the list of command arguments
void clear_cmdarg(void) {
  cmdcount = 0;
}

// Add an argument to the list of command arguments
void add_cmdarg(char *str) {
  if (cmdcount == MAXCMDARGS) {
    fprintf(stderr, "Out of space in cmdargs\n");
    Exit(1);
  }
  cmdarg[cmdcount++] = str;
}

// Return 1 if the string ends in '.'
// then the given character, 0 otherwise
int endswith(char *str, char ch) {
  int len = strlen(str);
  if (len < 2) return (0);
  if (str[len - 1] != ch) return (0);
  if (str[len - 2] != '.') return (0);
  return (1);
}

// Given a filename, open it for writing or Exit
FILE *fopenw(char *filename) {
  FILE *f = fopen(filename, "w");
  if (f == NULL) {
    fprintf(stderr, "Unable to write to file %s\n", filename);
    Exit(1);
  }
  return (f);
}

// Given a filename and a desired suffix,
// return the name of a temporary file
// which can be written to, or return NULL
// if a temporary file cannot be made
char *newtempfile(char *origname, char *suffix) {
  char *name;
  FILE *handle;

  // First attempt: just add the suffix to the original name
  name = (char *) malloc(strlen(origname) + strlen(suffix) + 1);
  if (name != NULL) {
    strcpy(name, origname);
    strcat(name, suffix);

    // Now try to open it
    handle = fopen(name, "w");
    if (handle != NULL) { fclose(handle); addtmpname(name); return (name); }
  }

  // That filename didn't work. Try one in the TMPDIR
  name = (char *) malloc(strlen(TMPDIR) + strlen(suffix) + 20);
  if (name == NULL)
    return (NULL);
  sprintf(name, "%s/%s_XXXXXX", TMPDIR, suffix);

  // Now try to open it
  handle = fopenw(name);
  if (handle != NULL) { fclose(handle); addtmpname(name); return (name); }
  return (NULL);
}

// Run the command with arguments in cmdarg[].
// Replace stdin/stdout by opening in/out as required.
// If the command doesn't Exit(0), stop.
void run_command(char *in, char *out) {
  int i, pid, wstatus;
  FILE *fh;

  if (verbose) {
    fprintf(stderr, "Doing: ");
    for (i = 0; cmdarg[i] != NULL; i++) fprintf(stderr, "%s ", cmdarg[i]);
    fprintf(stderr, "\n");
    if (in != NULL) fprintf(stderr, "  redirecting stdin from %s\n", in);
    if (out != NULL) fprintf(stderr, "  redirecting stdout to %s\n", out);
  }

  pid = fork();
  switch (pid) {
  case -1:
    fprintf(stderr, "fork failed\n");
    Exit(1);

    // Child process
  case 0:
    // Close stdin/stdout as required
    if (in != NULL) {
      fh = freopen(in, "r", stdin);
      if (fh == NULL) {
	fprintf(stderr, "Unable to freopen %s for reading\n", in);
	Exit(1);
      }
    }

    if (out != NULL) {
      fh = freopen(out, "w", stdout);
      if (fh == NULL) {
	fprintf(stderr, "Unable to freopen %s for writing\n", out);
	Exit(1);
      }
    }

    execvp(cmdarg[0], cmdarg);
    fprintf(stderr, "exec %s failed\n", cmdarg[0]);

    // The parent: wait for child to exit cleanly
  default:
    if (waitpid(pid, &wstatus, 0) == -1) {
      fprintf(stderr, "waitpid failed\n");
      Exit(1);
    }

    // Get the child's Exit status and get the parent
    // to Exit(1) if the Exit status was not zero
    if (WIFEXITED(wstatus)) {
      if (WEXITSTATUS(wstatus) != 0) Exit(1);
    } else {
      fprintf(stderr, "child phase didn't exit\n"); Exit(1);
    }

    // The child phase was successful
    return;
  }
}

// Pre-process the file using the C pre-processor
char *do_preprocess(char *name) {
  int i;
  char *tempname;

  // Build the command
  clear_cmdarg();
  add_cmdarg(phasecmd[CPP_PHASE]);
  for (i = 0; cppflags[i] != NULL; i++)
    add_cmdarg(cppflags[i]);
  for (i = 0; i < cppxindex; i++) {
    add_cmdarg("-D");
    add_cmdarg(cppextra[i]);
  }
  add_cmdarg(name);
  add_cmdarg(NULL);

  // If this is the last phase, use outname
  // as the output file, or stdout if NULL.
  if (last_phase == CPP_PHASE) {
    run_command(NULL, outname);
    Exit(0);
  }

  // Not the last phase, make a temp file
  tempname = newtempfile(initname, "_cpp");
  run_command(NULL, tempname);
  return (tempname);
}

// Assemble the given filename
char *do_assemble(char *name) {
  char *tempname;

  // If this is the last phase, use outname if
  // not NULL, or change the original file's suffix
  if (last_phase == ASM_PHASE) {
    if (outname == NULL)
      outname = alter_suffix('o');
    tempname = outname;
  } else {
    // Not the last phase, make a temp filename
    tempname = newtempfile(initname, "_o");
  }

  // Build and run the assembler command
  clear_cmdarg();
  add_cmdarg(phasecmd[ASM_PHASE]);
  add_cmdarg("-o");
  add_cmdarg(tempname);
  add_cmdarg(name);
  add_cmdarg(NULL);
  run_command(NULL, NULL);

  // Stop now if we are the last phase
  if (last_phase == ASM_PHASE)
    Exit(0);

  return (tempname);
}

// Run several compiler phases to take a
// pre-processed C file to an assembly file
char *do_compile(char *name) {
  char *tokname, *symname, *astname;
  char *idxname, *qbename, *asmname;

  // We need to run the scanner, the parser
  // and the code generator. Get a temp filename
  // for the scanner's output.
  tokname = newtempfile(initname, "_tok");

  // Build and run the scanner command
  clear_cmdarg();
  add_cmdarg(phasecmd[TOK_PHASE]);
  add_cmdarg(NULL);
  run_command(name, tokname);

  // Get temp filenames for the parser's output
  symname = newtempfile(initname, "_sym");
  astname = newtempfile(initname, "_ast");

  // Build and run the parser command
  clear_cmdarg();
  add_cmdarg(phasecmd[PARSE_PHASE]);
  add_cmdarg(symname);
  add_cmdarg(astname);
  add_cmdarg(NULL);
  run_command(tokname, NULL);

  // Get some temporary filenames even
  // if we don't use them.
  idxname = newtempfile(initname, "_idx");
  qbename = newtempfile(initname, "_qbe");
  asmname = newtempfile(initname, "_s");

  // If this phase (compile to assembly) is
  // the last, use outname if not NULL,
  // or change the original file's suffix.
  if (last_phase == GEN_PHASE) {
    if (outname == NULL)
      outname = alter_suffix('s');
    asmname = outname;
  }

  // Before we run the code generator, see
  // if the next (QBE or peephole) phase exists.
  // If not, we go straight to assembly code, so
  // change the output file's name
  if (phasecmd[QBEPEEP_PHASE] == NULL) {
    qbename = asmname;
  }

  // Build and run the code generator command
  clear_cmdarg();
  add_cmdarg(phasecmd[GEN_PHASE]);
  add_cmdarg(symname);
  add_cmdarg(astname);
  add_cmdarg(idxname);
  add_cmdarg(NULL);
  run_command(NULL, qbename);

  // Build and run the QBE command or the
  // peephole optmiser if needed
  if (phasecmd[QBEPEEP_PHASE] != NULL) {
    clear_cmdarg();
    add_cmdarg(phasecmd[QBEPEEP_PHASE]);
    if (cpu== CPU_QBE) {
      add_cmdarg("-o");
      add_cmdarg(asmname);
      add_cmdarg(qbename);
    }
    if (cpu== CPU_6809) {
      add_cmdarg("-o");
      add_cmdarg(asmname);
      add_cmdarg(qbename);
      add_cmdarg(LIB6809DIR "/rules.6809");
    }
    add_cmdarg(NULL);
    run_command(NULL, NULL);
  }

  // Stop now if we are the last phase
  if (last_phase == GEN_PHASE)
    Exit(0);

  return (asmname);
}

// Link the final executable with all
// the object files
void do_link(void) {
  int i;
  struct filelist *this;

  // Build the command
  clear_cmdarg();
  add_cmdarg(phasecmd[LINK_PHASE]);
  add_cmdarg("-o");
  add_cmdarg(outname);

  // Insert any files that must come first
  for (i = 0; preobjs[i] != NULL; i++)
    add_cmdarg(preobjs[i]);

  // Now add on all the object files and library names
  for (this = Objhead; this != NULL; this = this->next)
    add_cmdarg(this->name);

  // Insert any files that must come at the end
  for (i = 0; postobjs[i] != NULL; i++)
    add_cmdarg(postobjs[i]);
  add_cmdarg(NULL);
  run_command(NULL, NULL);
}

// Given a CPU/platform name, change the phase
// programs and object files
void set_phaseprograms(char *cpuname) {
  if (!strcmp(cpuname, "qbe")) {
    phasecmd = qbephasecmd;
    cppflags = qbecppflags;
    preobjs = qbepreobjs;
    postobjs = qbepostobjs;
    cpu= CPU_QBE;
    return;
  }
  if (!strcmp(cpuname, "6809")) {
    phasecmd = phasecmd6809;
    cppflags = cppflags6809;
    preobjs = preobjs6809;
    postobjs = postobjs6809;
    cpu= CPU_6809;
    return;
  }
  fprintf(stderr, "Unknown CPU/patform: %s\n", cpuname);
  Exit(1);
}


// Print out a usage if started incorrectly
static void usage(char *prog) {
  fprintf(stderr, "Usage: %s [-vcESX] [-D ...] [-m CPU] [-o outfile] file [file ...]\n",
	  prog);
  fprintf(stderr,
	  "       -v give verbose output of the compilation stages\n");
  fprintf(stderr, "       -c generate object files but don't link them\n");
  fprintf(stderr, "       -E pre-process the file, output on stdout\n");
  fprintf(stderr, "       -S generate assembly files but don't link them\n");
  fprintf(stderr, "       -X keep temporary files for debugging\n");
  fprintf(stderr, "       -D ..., set a pre-processor define\n");
  fprintf(stderr, "       -m CPU, set the CPU e.g. -m 6809, -m qbe\n");
  fprintf(stderr, "       -o outfile, produce the outfile executable file\n");
  Exit(1);
}


// Main program: check arguments, or print a usage
// if we don't have any arguments.

int main(int argc, char **argv) {
  int i, opt;

  phasecmd = qbephasecmd;
  cppflags = qbecppflags;
  preobjs = qbepreobjs;
  postobjs = qbepostobjs;

  // Get the options
  if (argc < 2)
    usage(argv[0]);
  while ((opt = getopt(argc, argv, "vcESXo:m:D:")) != -1) {
    switch (opt) {
    case 'v': verbose = 1; break;
    case 'c': last_phase = ASM_PHASE; break;
    case 'E': last_phase = CPP_PHASE; break;
    case 'S': last_phase = GEN_PHASE; break;
    case 'X': keep_tempfiles = 1; break;
    case 'm': set_phaseprograms(optarg); break;
    case 'o': outname = optarg; break;
    case 'D': if (cppxindex >= MAXCPPEXTRA) {
		fprintf(stderr, "Too many -D arguments\n"); Exit(1);
	      }
	      cppextra[cppxindex]= optarg; cppxindex++; break;
    }
  }

  // Now process the filenames after the arguments
  if (optind >= argc) usage(argv[0]);
  for (i = optind; i < argc; i++) {
    initname = argv[i];

    if (endswith(argv[i], 'c')) {
      // A C source file, do all major phases
      addobjname(do_assemble(do_compile(do_preprocess(argv[i]))));
    } else if (endswith(argv[i], 's')) {
      // An assembly file, just assemble
      addobjname(do_assemble(argv[i]));
    } else if (endswith(argv[i], 'o')) {
      // Add object files to the list
      addobjname(argv[i]);
    } else {
      fprintf(stderr, "Input file with unrecognised suffix: %s\n", argv[i]);
      usage(argv[0]);
    }
  }

  // Now link all the object files together
  if (outname == NULL) outname = AOUT;
  do_link();

  Exit(0);
  return (0);
}
