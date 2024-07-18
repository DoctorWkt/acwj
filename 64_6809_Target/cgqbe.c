#include "defs.h"
#include "data.h"
#include "gen.h"
#include "misc.h"
#include "types.h"
#include "target.h"
#include "cg.h"

// Code generator for x86-64 using the QBE intermediate language.
// Copyright (c) 2019 Warren Toomey, GPL3

// We have to keep a list of literal strings as we can't generate
// them in the middle of code
struct litlist {
  char *val;
  int label;
  struct litlist *next;
};

struct litlist *Strlithead;
struct litlist *Strlittail;

// Switch to the text segment
void cgtextseg() {
}

// Switch to the data segment
void cgdataseg() {
}

// Switch to the literal segment
void cglitseg() {
}

// Free registers/temporaries
void cgfreeallregs(int keepreg) {
}

void cgfreereg(int reg) {
}

// Given a scalar type value, return the
// character that matches the QBE type.
// Because chars are stored on the stack,
// we can return 'w' for P_CHAR.
static int cgprimtype(int type) {
  if (ptrtype(type))
    return ('l');
  switch (type) {
    case P_VOID:
      return (' ');
    case P_CHAR:
      return ('w');
    case P_INT:
      return ('w');
    case P_LONG:
      return ('l');
    default:
      fatald("Bad type in cgprimtype:", type);
  }
  return (0);			// Keep -Wall happy
}

// Allocate a QBE temporary
static int nexttemp = 0;
int cgalloctemp(void) {
  return (++nexttemp);
}

int cgallocreg(int type) {
  return(cgalloctemp());
}

// Print out the assembly preamble
// for one output file
void cgpreamble() {
  Strlithead= NULL;
  Strlittail= NULL;
}

// Print out any global string literals
static void cgmakeglobstrs();
void cgpostamble() {
  cgmakeglobstrs();
}

// Boolean flag: has there been a switch statement
// in this function yet?
static int used_switch;

// Print out a function preamble
void cgfuncpreamble(struct symtable *sym) {
  char *name = sym->name;
  struct symtable *parm, *locvar;
  int size, bigsize;
  int label;

  // Output the function's name and return type
  if (sym->class == V_GLOBAL)
    fprintf(Outfile, "export ");
  fprintf(Outfile, "function %c $%s(", cgprimtype(sym->type), name);

  // Output the parameter names and types. For any parameters which
  // need addresses, change their name as we copy their value below
  for (parm = sym->member; parm != NULL; parm = parm->next) {
    if (parm->class==V_LOCAL) break;

    // Ugly. Make all params have a address
    parm->st_hasaddr = 1;
    if (parm->st_hasaddr == 1)
      fprintf(Outfile, "%c %%.p%s, ", cgprimtype(parm->type), parm->name);
    else
      fprintf(Outfile, "%c %%%s, ", cgprimtype(parm->type), parm->name);
  }
  fprintf(Outfile, ") {\n");

  // Get a label for the function start
  label = genlabel();
  cglabel(label);

  // For any parameters which need addresses, allocate memory
  // on the stack for them. QBE won't let us do alloc1, so
  // we allocate 4 bytes for chars. Copy the value from the
  // parameter to the new memory location.
  // of the parameter
  for (parm = sym->member; parm != NULL; parm = parm->next) {
    if (parm->class==V_LOCAL) break;

    // Ugly. Make all params have a address
    parm->st_hasaddr = 1;
    if (parm->st_hasaddr == 1) {
      size = cgprimsize(parm->type);
      bigsize = (size == 1) ? 4 : size;
      fprintf(Outfile, "  %%%s =l alloc%d 1\n", parm->name, bigsize);

      // Copy to the allocated memory
      switch (size) {
	case 1:
	  fprintf(Outfile, "  storeb %%.p%s, %%%s\n", parm->name, parm->name);
	  break;
	case 4:
	  fprintf(Outfile, "  storew %%.p%s, %%%s\n", parm->name, parm->name);
	  break;
	case 8:
	  fprintf(Outfile, "  storel %%.p%s, %%%s\n", parm->name, parm->name);
      }
    }
  }

  // Allocate memory for any local variables that need to be on the
  // stack. There are two reasons for this. The first is for locals
  // where their address is used. The second is for char variables
  // We need to do this as QBE can only truncate down to 8 bits
  // for locations in memory.
  // Note: locals come after parameters in the member list.
  for (locvar = parm; locvar != NULL; locvar = locvar->next) {
    if (locvar->st_hasaddr == 1) {
      // Get the total size for all elements (if an array).
      // Round up to the nearest multiple of 8, to ensure that
      // pointers are aligned on 8-byte boundaries
      size = locvar->size * locvar->nelems;
      size = (size + 7) >> 3;
      fprintf(Outfile, "  %%%s =l alloc8 %d\n", locvar->name, size);
    } else if (locvar->type == P_CHAR) {
      locvar->st_hasaddr = 1;
      fprintf(Outfile, "  %%%s =l alloc4 1\n", locvar->name);
    }
  }

  used_switch = 0;		// We haven't output the switch handling code yet
}

// Print out a function postamble
void cgfuncpostamble(struct symtable *sym) {
  cglabel(sym->st_endlabel);

  // Return a value if the function's type isn't void
  if (sym->type != P_VOID)
    fprintf(Outfile, "  ret %%.ret\n}\n");
  else
    fprintf(Outfile, "  ret\n}\n");
}

// Load an integer literal value into a temporary.
// Return the number of the temporary.
int cgloadint(int value, int type) {
  // Get a new temporary
  int t = cgalloctemp();

  fprintf(Outfile, "  %%.t%d =%c copy %d\n", t, cgprimtype(type), value);
  return (t);
}

// Load a value from a variable into a temporary.
// Return the number of the temporary. If the
// operation is pre- or post-increment/decrement,
// also perform this action.
int cgloadvar(struct symtable *sym, int op) {
  int r, posttemp, offset = 1;
  char qbeprefix;

  // Get a new temporary
  r = cgalloctemp();

  // If the symbol is a pointer, use the size
  // of the type that it points to as any
  // increment or decrement. If not, it's one.
  if (ptrtype(sym->type))
    offset = typesize(value_at(sym->type), sym->ctype);

  // Negate the offset for decrements
  if (op == A_PREDEC || op == A_POSTDEC)
    offset = -offset;

  // Get the relevant QBE prefix for the symbol
  qbeprefix = ((sym->class == V_GLOBAL) || (sym->class == V_STATIC) ||
	       (sym->class == V_EXTERN)) ? (char)'$' : (char)'%';

  // If we have a pre-operation
  if (op == A_PREINC || op == A_PREDEC) {
    if (sym->st_hasaddr || qbeprefix == '$') {
      // Get a new temporary
      posttemp = cgalloctemp();
      switch (sym->size) {
	case 1:
	  fprintf(Outfile, "  %%.t%d =w loadub %c%s\n", posttemp, qbeprefix,
		  sym->name);
	  fprintf(Outfile, "  %%.t%d =w add %%.t%d, %d\n", posttemp, posttemp,
		  offset);
	  fprintf(Outfile, "  storeb %%.t%d, %c%s\n", posttemp, qbeprefix,
		  sym->name);
	  break;
	case 4:
	  fprintf(Outfile, "  %%.t%d =w loadsw %c%s\n", posttemp, qbeprefix,
		  sym->name);
	  fprintf(Outfile, "  %%.t%d =w add %%.t%d, %d\n", posttemp, posttemp,
		  offset);
	  fprintf(Outfile, "  storew %%.t%d, %c%s\n", posttemp, qbeprefix,
		  sym->name);
	  break;
	case 8:
	  fprintf(Outfile, "  %%.t%d =l loadl %c%s\n", posttemp, qbeprefix,
		  sym->name);
	  fprintf(Outfile, "  %%.t%d =l add %%.t%d, %d\n", posttemp, posttemp,
		  offset);
	  fprintf(Outfile, "  storel %%.t%d, %c%s\n", posttemp, qbeprefix,
		  sym->name);
      }
    } else
      fprintf(Outfile, "  %c%s =%c add %c%s, %d\n",
	      qbeprefix, sym->name, cgprimtype(sym->type), qbeprefix,
	      sym->name, offset);
  }
  // Now load the output temporary with the value
  if (sym->st_hasaddr || qbeprefix == '$') {
    switch (sym->size) {
      case 1:
	fprintf(Outfile, "  %%.t%d =w loadub %c%s\n", r, qbeprefix,
		sym->name);
	break;
      case 4:
	fprintf(Outfile, "  %%.t%d =w loadsw %c%s\n", r, qbeprefix,
		sym->name);
	break;
      case 8:
	fprintf(Outfile, "  %%.t%d =l loadl %c%s\n", r, qbeprefix, sym->name);
    }
  } else
    fprintf(Outfile, "  %%.t%d =%c copy %c%s\n",
	    r, cgprimtype(sym->type), qbeprefix, sym->name);

  // If we have a post-operation
  if (op == A_POSTINC || op == A_POSTDEC) {
    if (sym->st_hasaddr || qbeprefix == '$') {
      // Get a new temporary
      posttemp = cgalloctemp();
      switch (sym->size) {
	case 1:
	  fprintf(Outfile, "  %%.t%d =w loadub %c%s\n", posttemp, qbeprefix,
		  sym->name);
	  fprintf(Outfile, "  %%.t%d =w add %%.t%d, %d\n", posttemp, posttemp,
		  offset);
	  fprintf(Outfile, "  storeb %%.t%d, %c%s\n", posttemp, qbeprefix,
		  sym->name);
	  break;
	case 4:
	  fprintf(Outfile, "  %%.t%d =w loadsw %c%s\n", posttemp, qbeprefix,
		  sym->name);
	  fprintf(Outfile, "  %%.t%d =w add %%.t%d, %d\n", posttemp, posttemp,
		  offset);
	  fprintf(Outfile, "  storew %%.t%d, %c%s\n", posttemp, qbeprefix,
		  sym->name);
	  break;
	case 8:
	  fprintf(Outfile, "  %%.t%d =l loadl %c%s\n", posttemp, qbeprefix,
		  sym->name);
	  fprintf(Outfile, "  %%.t%d =l add %%.t%d, %d\n", posttemp, posttemp,
		  offset);
	  fprintf(Outfile, "  storel %%.t%d, %c%s\n", posttemp, qbeprefix,
		  sym->name);
      }
    } else
      fprintf(Outfile, "  %c%s =%c add %c%s, %d\n",
	      qbeprefix, sym->name, cgprimtype(sym->type), qbeprefix,
	      sym->name, offset);
  }
  // Return the temporary with the value
  return (r);
}

// Given the label number of a global string,
// load its address into a new temporary
int cgloadglobstr(int label) {
  // Get a new temporary
  int r = cgalloctemp();
  fprintf(Outfile, "  %%.t%d =l copy $L%d\n", r, label);
  return (r);
}

// Add two temporaries together and return
// the number of the temporary with the result
int cgadd(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c add %%.t%d, %%.t%d\n",
	  r1, cgprimtype(type), r1, r2);
  return (r1);
}

// Subtract the second temporary from the first and
// return the number of the temporary with the result
int cgsub(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c sub %%.t%d, %%.t%d\n",
	  r1, cgprimtype(type), r1, r2);
  return (r1);
}

// Multiply two temporaries together and return
// the number of the temporary with the result
int cgmul(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c mul %%.t%d, %%.t%d\n",
	  r1, cgprimtype(type), r1, r2);
  return (r1);
}

// Divide the first temporary by the second and
// return the number of the temporary with the result
int cgdiv(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c div %%.t%d, %%.t%d\n",
	    r1, cgprimtype(type), r1, r2);
  return (r1);
}

// Modulo the first temporary by the second and
// return the number of the temporary with the result
int cgmod(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c rem %%.t%d, %%.t%d\n",
	    r1, cgprimtype(type), r1, r2);
  return (r1);
}

// Bitwise AND two temporaries
int cgand(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c and %%.t%d, %%.t%d\n",
	  r1, cgprimtype(type), r1, r2);
  return (r1);
}

// Bitwise OR two temporaries
int cgor(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c or %%.t%d, %%.t%d\n",
	  r1, cgprimtype(type), r1, r2);
  return (r1);
}

// Bitwise XOR two temporaries
int cgxor(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c xor %%.t%d, %%.t%d\n",
	  r1, cgprimtype(type), r1, r2);
  return (r1);
}

// Shift left r1 by r2 bits
int cgshl(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c shl %%.t%d, %%.t%d\n",
	  r1, cgprimtype(type), r1, r2);
  return (r1);
}

// Shift right r1 by r2 bits
int cgshr(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c shr %%.t%d, %%.t%d\n",
	  r1, cgprimtype(type), r1, r2);
  return (r1);
}

// Negate a temporary's value
int cgnegate(int r, int type) {
  fprintf(Outfile, "  %%.t%d =%c sub 0, %%.t%d\n", r, cgprimtype(type), r);
  return (r);
}

// Invert a temporary's value
int cginvert(int r, int type) {
  fprintf(Outfile, "  %%.t%d =%c xor %%.t%d, -1\n", r, cgprimtype(type), r);
  return (r);
}

// Logically negate a temporary's value
int cglognot(int r, int type) {
  int q = cgprimtype(type);
  fprintf(Outfile, "  %%.t%d =%c ceq%c %%.t%d, 0\n", r, q, q, r);
  return (r);
}

// Load a boolean value (only 0 or 1)
// into the given temporary. Allocate a
// temporary if r is NOREG
int cgloadboolean(int r, int val, int type) {
  if (r==NOREG) r= cgalloctemp();
  fprintf(Outfile, "  %%.t%d =%c copy %d\n", r, cgprimtype(type), val);
  return(r);
}

// Convert an integer value to a boolean value for
// a TOBOOL operation. Jump if true if it's an IF, 
// WHILE operation. Jump if false if it's 
// a LOGOR operation.
int cgboolean(int r, int op, int label, int type) {
  // Get a label for the next instruction
  int label2 = genlabel();

  // Get a new temporary for the comparison
  int r2 = cgalloctemp();

  // Convert temporary to boolean value
  fprintf(Outfile, "  %%.t%d =l cne%c %%.t%d, 0\n", r2, cgprimtype(type), r);

  switch (op) {
    case A_IF:
    case A_WHILE:
    case A_TERNARY:
    case A_LOGAND:
      fprintf(Outfile, "  jnz %%.t%d, @L%d, @L%d\n", r2, label2, label);
      break;
    case A_LOGOR:
      fprintf(Outfile, "  jnz %%.t%d, @L%d, @L%d\n", r2, label, label2);
      break;
  }

  // Output the label for the next instruction
  cglabel(label2);
  return (r2);
}

// Call a function with the given symbol id.
// Return the temprary with the result
int cgcall(struct symtable *sym, int numargs, int *arglist, int *typelist) {
  int outr;
  int i;

  // Get a new temporary for the return result
  outr = cgalloctemp();

  // Call the function
  if (sym->type == P_VOID)
    fprintf(Outfile, "  call $%s(", sym->name);
  else
    fprintf(Outfile, "  %%.t%d =%c call $%s(", outr, cgprimtype(sym->type),
	    sym->name);

  // Output the list of arguments
  for (i = numargs - 1; i >= 0; i--) {
    fprintf(Outfile, "%c %%.t%d, ", cgprimtype(typelist[i]), arglist[i]);
  }
  fprintf(Outfile, ")\n");

  return (outr);
}

// Shift a temporary left by a constant. As we only
// use this for address calculations, extend the
// type to be a QBE 'l' if required
int cgshlconst(int r, int val, int type) {
  int r2 = cgalloctemp();
  int r3 = cgalloctemp();

  if (cgprimsize(type) < 8) {
    fprintf(Outfile, "  %%.t%d =l extsw %%.t%d\n", r2, r);
    fprintf(Outfile, "  %%.t%d =l shl %%.t%d, %d\n", r3, r2, val);
  } else
    fprintf(Outfile, "  %%.t%d =l shl %%.t%d, %d\n", r3, r, val);
  return (r3);
}

// Store a temporary's value into a global variable
int cgstorglob(int r, struct symtable *sym) {

  // We can store to bytes in memory
  int q = cgprimtype(sym->type);
  if (sym->type == P_CHAR)
    q = 'b';

  fprintf(Outfile, "  store%c %%.t%d, $%s\n", q, r, sym->name);
  return (r);
}

// Store a temporary's value into a local variable
int cgstorlocal(int r, struct symtable *sym) {

  // If the variable is on the stack, use store instructions
  if (sym->st_hasaddr) {
    fprintf(Outfile, "  store%c %%.t%d, %%%s\n",
	    cgprimtype(sym->type), r, sym->name);
  } else {
    fprintf(Outfile, "  %%%s =%c copy %%.t%d\n",
	    sym->name, cgprimtype(sym->type), r);
  }
  return (r);
}

// Generate a global symbol but not functions
void cgglobsym(struct symtable *node) {
  int size, type;
  int initvalue;
  int i;

  if (node == NULL)
    return;
  if (node->stype == S_FUNCTION)
    return;

  // Get the size of the variable (or its elements if an array)
  // and the type of the variable
  if (node->stype == S_ARRAY) {
    size = typesize(value_at(node->type), node->ctype);
    type = value_at(node->type);
  } else {
    size = node->size;
    type = node->type;
  }

  // Generate the global identity and the label
  cgdataseg();
  if (node->class == V_GLOBAL)
    fprintf(Outfile, "export ");
  if (node->ctype==NULL)
    fprintf(Outfile, "data $%s = align %d { ", node->name, cgprimsize(type));
  else
    fprintf(Outfile, "data $%s = align 8 { ", node->name);

  // Output space for one or more elements
  for (i = 0; i < node->nelems; i++) {

    // Get any initial value
    initvalue = 0;
    if (node->initlist != NULL)
      initvalue = node->initlist[i];

    // Generate the space for this type
    switch (size) {
      case 1:
	fprintf(Outfile, "b %d, ", initvalue);
	break;
      case 4:
	fprintf(Outfile, "w %d, ", initvalue);
	break;
      case 8:
	// Generate the pointer to a string literal. Treat a zero value
	// as actually zero, not the label L0
	if (node->initlist != NULL && type == pointer_to(P_CHAR)
	    && initvalue != 0)
	  fprintf(Outfile, "l $L%d, ", initvalue);
	else
	  fprintf(Outfile, "l %d, ", initvalue);
	break;
      default:
	fprintf(Outfile, "z %d, ", size);
    }
  }
  fprintf(Outfile, "}\n");
}

// Stash a global string for later output
void cgglobstr(int l, char *strvalue) {
  struct litlist *this;

  this= (struct litlist *)malloc(sizeof(struct litlist));
  this->val= strdup(strvalue);
  this->label= l;
  this->next= NULL;
  if (Strlithead==NULL) {
    Strlithead= Strlittail= this;
  } else {
    Strlittail->next= this; Strlittail= this;
  }
}

// Generate all the global strings and their labels
static void cgmakeglobstrs() {
  struct litlist *this;
  char *cptr;

  for (this= Strlithead; this!=NULL; this=this->next) {

    fprintf(Outfile, "data $L%d = { ", this->label);
    for (cptr = this->val; *cptr; cptr++) {
      fprintf(Outfile, "b %d, ", *cptr);
    }
    fprintf(Outfile, " b 0 }\n");
  }
}

// NUL terminate a global string
void cgglobstrend(void) {
}

// List of comparison instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *cmplist[] = { "ceq", "cne", "cslt", "csgt", "csle", "csge" };

// Compare two temporaries and set if true.
int cgcompare_and_set(int ASTop, int r1, int r2, int type) {
  int r3;
  int q = cgprimtype(type);

  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in cgcompare_and_set()");

  // Get a new temporary for the comparison
  r3 = cgalloctemp();

  fprintf(Outfile, "  %%.t%d =%c %s%c %%.t%d, %%.t%d\n",
	  r3, q, cmplist[ASTop - A_EQ], q, r1, r2);
  return (r3);
}

// Generate a label
void cglabel(int l) {
  fprintf(Outfile, "@L%d\n", l);
}

// Generate a jump to a label
void cgjump(int l) {
  int label;

  fprintf(Outfile, "  jmp @L%d\n", l);

  // Print out a bogus label. This prevents the output
  // having two adjacent jumps which QBE doesn't like.
  label = genlabel();
  cglabel(label);
}

// List of inverted jump instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *invcmplist[] = { "cne", "ceq", "csge", "csle", "csgt", "cslt" };

// Compare two temporaries and jump if false.
// Jump if true if the parent op is A_LOGOR.
int cgcompare_and_jump(int ASTop, int parentASTop,
				int r1, int r2, int label, int type) {
  int label2;
  int r3;
  int q = cgprimtype(type);
  char *cmpop;

  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in cgcompare_and_set()");

  cmpop= invcmplist[ASTop - A_EQ];
  if (parentASTop == A_LOGOR)
    cmpop= cmplist[ASTop - A_EQ];

  // Get a label for the next instruction
  label2 = genlabel();

  // Get a new temporary for the comparison
  r3 = cgalloctemp();

  fprintf(Outfile, "  %%.t%d =%c %s%c %%.t%d, %%.t%d\n",
	  r3, q, cmpop, q, r1, r2);
  fprintf(Outfile, "  jnz %%.t%d, @L%d, @L%d\n", r3, label, label2);
  cglabel(label2);
  return (NOREG);
}

// Widen the value in the temporary from the old
// to the new type, and return a temporary with
// this new value
int cgwiden(int r, int oldtype, int newtype) {
  int oldq = cgprimtype(oldtype);
  int newq = cgprimtype(newtype);

  // Get a new temporary
  int t = cgalloctemp();

  switch (oldtype) {
    case P_CHAR:
      fprintf(Outfile, "  %%.t%d =%c extub %%.t%d\n", t, newq, r);
      break;
    default:
      fprintf(Outfile, "  %%.t%d =%c exts%c %%.t%d\n", t, newq, oldq, r);
  }
  return (t);
}

// Generate code to return a value from a function
void cgreturn(int r, struct symtable *sym) {
  // Only return a value if we have a value to return
  if (r != NOREG)
    fprintf(Outfile, "  %%.ret =%c copy %%.t%d\n", cgprimtype(sym->type), r);

  cgjump(sym->st_endlabel);
}

// Generate code to load the address of an
// identifier. Return a new temporary
int cgaddress(struct symtable *sym) {
  int r = cgalloctemp();
  char qbeprefix = ((sym->class == V_GLOBAL) || (sym->class == V_STATIC) ||
		    (sym->class == V_EXTERN)) ? (char)'$' : (char)'%';

  fprintf(Outfile, "  %%.t%d =l copy %c%s\n", r, qbeprefix, sym->name);
  return (r);
}

// Dereference a pointer to get the value
// it points at into a new temporary
int cgderef(int r, int type) {
  // Get the type that we are pointing to
  int newtype = value_at(type);
  // Now get the size of this type
  int size = cgprimsize(newtype);
  // Get temporary for the return result
  int ret = cgalloctemp();

  switch (size) {
    case 1:
      fprintf(Outfile, "  %%.t%d =w loadub %%.t%d\n", ret, r);
      break;
    case 4:
      fprintf(Outfile, "  %%.t%d =w loadsw %%.t%d\n", ret, r);
      break;
    case 8:
      fprintf(Outfile, "  %%.t%d =l loadl %%.t%d\n", ret, r);
      break;
    default:
      fatald("Can't cgderef on type:", type);
  }
  return (ret);
}

// Store through a dereferenced pointer
int cgstorderef(int r1, int r2, int type) {
  // Get the size of the type
  int size = cgprimsize(type);

  switch (size) {
    case 1:
      fprintf(Outfile, "  storeb %%.t%d, %%.t%d\n", r1, r2);
      break;
    case 4:
      fprintf(Outfile, "  storew %%.t%d, %%.t%d\n", r1, r2);
      break;
    case 8:
      fprintf(Outfile, "  storel %%.t%d, %%.t%d\n", r1, r2);
      break;
    default:
      fatald("Can't cgstoderef on type:", type);
  }
  return (r1);
}

// Generate code to compare each switch value
// and jump to the appropriate case label.
void cgswitch(int reg, int casecount, int toplabel,
	      int *caselabel, int *caseval, int defaultlabel) {
  int i, label;
  int rval, rcmp;

  // Get two temporaries for the case value and the comparison
  rval= cgalloctemp();
  rcmp= cgalloctemp();

  // Output the label at the top of the code
  cglabel(toplabel);

  for (i = 0; i < casecount; i++) {
    // Get a label for the code when we skip this case
    label= genlabel();

    // Load the case value 
    fprintf(Outfile, "  %%.t%d =w copy %d\n", rval, caseval[i]);

    // Compare the temporary against the case value
    fprintf(Outfile, "  %%.t%d =w ceqw %%.t%d, %%.t%d\n", rcmp, reg, rval);

    // Jump either to the next comparison or the case code
    fprintf(Outfile, "  jnz %%.t%d, @L%d, @L%d\n", rcmp, caselabel[i], label);
    cglabel(label);
  }

  // No case matched, jump to the default label
  cgjump(defaultlabel);
}

// Move value between temporaries
void cgmove(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c copy %%.t%d\n", r2, cgprimtype(type), r1);
}

// Output a gdb directive to say on which
// source code line number the following
// assembly code came from
void cglinenum(int line) {
  // fprintf(Outfile, "\t.loc 1 %d 0\n", line);
}

// Change a temporary value from its old
// type to a new type.
int cgcast(int t, int oldtype, int newtype) {
  // Get temporary for the return result
  int ret = cgalloctemp();
  int oldsize, newsize;
  int qnew;

  // If the new type is a pointer
  if (ptrtype(newtype)) {
    // Nothing to do if the old type is also a pointer
    if (ptrtype(oldtype))
      return (t);
    // Otherwise, widen from a primitive type to a pointer
    return (cgwiden(t, oldtype, newtype));
  }

  // New type is not a pointer
  // Get the new QBE type
  // and the type sizes in bytes
  qnew = cgprimtype(newtype);
  oldsize = cgprimsize(oldtype);
  newsize = cgprimsize(newtype);

  // Nothing to do if the two are the same size
  if (newsize == oldsize)
    return (t);

  // If the new size is smaller, we can copy and QBE will truncate it,
  // otherwise use the QBE cast operation
  if (newsize < oldsize)
    fprintf(Outfile, " %%.t%d =%c copy %%.t%d\n", ret, qnew, t);
  else
    fprintf(Outfile, " %%.t%d =%c cast %%.t%d\n", ret, qnew, t);
  return (ret);
}
