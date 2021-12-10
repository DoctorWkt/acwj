#include "defs.h"
#include "data.h"
#include "decl.h"

// Code generator for x86-64 using the QBE intermediate language.
// Copyright (c) 2019 Warren Toomey, GPL3

// Switch to the text segment
void cgtextseg() {
}

// Switch to the data segment
void cgdataseg() {
}

// Given a scalar type value, return the
// character that matches the QBE type.
// Because chars are stored on the stack,
// we can return 'w' for P_CHAR.
char cgqbetype(int type) {
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
      fatald("Bad type in cgqbetype:", type);
  }
  return (0);			// Keep -Wall happy
}

// Given a scalar type value, return the
// size of the QBE type in bytes.
int cgprimsize(int type) {
  if (ptrtype(type))
    return (8);
  switch (type) {
    case P_CHAR:
      return (1);
    case P_INT:
      return (4);
    case P_LONG:
      return (8);
    default:
      fatald("Bad type in cgprimsize:", type);
  }
  return (0);			// Keep -Wall happy
}

// Given a scalar type, an existing memory offset
// (which hasn't been allocated to anything yet)
// and a direction (1 is up, -1 is down), calculate
// and return a suitably aligned memory offset
// for this scalar type. This could be the original
// offset, or it could be above/below the original
int cgalign(int type, int offset, int direction) {
  int alignment;

  // We don't need to do this on x86-64, but let's
  // align chars on any offset and align ints/pointers
  // on a 4-byte alignment
  switch (type) {
    case P_CHAR:
      break;
    default:
      // Align whatever we have now on a 4-byte alignment.
      // I put the generic code here so it can be reused elsewhere.
      alignment = 4;
      offset = (offset + direction * (alignment - 1)) & ~(alignment - 1);
  }
  return (offset);
}

// Allocate a QBE temporary
static int nexttemp = 0;
int cgalloctemp(void) {
  return (++nexttemp);
}

// Print out the assembly preamble
// for one output file
void cgpreamble(char *filename) {
}

// Nothing to do for the end of a file
void cgpostamble() {
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
  if (sym->class == C_GLOBAL)
    fprintf(Outfile, "export ");
  fprintf(Outfile, "function %c $%s(", cgqbetype(sym->type), name);

  // Output the parameter names and types. For any parameters which
  // need addresses, change their name as we copy their value below
  for (parm = sym->member; parm != NULL; parm = parm->next) {
    if (parm->st_hasaddr == 1)
      fprintf(Outfile, "%c %%.p%s, ", cgqbetype(parm->type), parm->name);
    else
      fprintf(Outfile, "%c %%%s, ", cgqbetype(parm->type), parm->name);
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
  // for locations in memory
  for (locvar = Loclhead; locvar != NULL; locvar = locvar->next) {
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

  fprintf(Outfile, "  %%.t%d =%c copy %d\n", t, cgqbetype(type), value);
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
  qbeprefix = ((sym->class == C_GLOBAL) || (sym->class == C_STATIC) ||
	       (sym->class == C_EXTERN)) ? '$' : '%';

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
	      qbeprefix, sym->name, cgqbetype(sym->type), qbeprefix,
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
	    r, cgqbetype(sym->type), qbeprefix, sym->name);

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
	      qbeprefix, sym->name, cgqbetype(sym->type), qbeprefix,
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
	  r1, cgqbetype(type), r1, r2);
  return (r1);
}

// Subtract the second temporary from the first and
// return the number of the temporary with the result
int cgsub(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c sub %%.t%d, %%.t%d\n",
	  r1, cgqbetype(type), r1, r2);
  return (r1);
}

// Multiply two temporaries together and return
// the number of the temporary with the result
int cgmul(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c mul %%.t%d, %%.t%d\n",
	  r1, cgqbetype(type), r1, r2);
  return (r1);
}

// Divide or modulo the first temporary by the second and
// return the number of the temporary with the result
int cgdivmod(int r1, int r2, int op, int type) {
  if (op == A_DIVIDE)
    fprintf(Outfile, "  %%.t%d =%c div %%.t%d, %%.t%d\n",
	    r1, cgqbetype(type), r1, r2);
  else
    fprintf(Outfile, "  %%.t%d =%c rem %%.t%d, %%.t%d\n",
	    r1, cgqbetype(type), r1, r2);
  return (r1);
}

// Bitwise AND two temporaries
int cgand(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c and %%.t%d, %%.t%d\n",
	  r1, cgqbetype(type), r1, r2);
  return (r1);
}

// Bitwise OR two temporaries
int cgor(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c or %%.t%d, %%.t%d\n",
	  r1, cgqbetype(type), r1, r2);
  return (r1);
}

// Bitwise XOR two temporaries
int cgxor(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c xor %%.t%d, %%.t%d\n",
	  r1, cgqbetype(type), r1, r2);
  return (r1);
}

// Shift left r1 by r2 bits
int cgshl(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c shl %%.t%d, %%.t%d\n",
	  r1, cgqbetype(type), r1, r2);
  return (r1);
}

// Shift right r1 by r2 bits
int cgshr(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c shr %%.t%d, %%.t%d\n",
	  r1, cgqbetype(type), r1, r2);
  return (r1);
}

// Negate a temporary's value
int cgnegate(int r, int type) {
  fprintf(Outfile, "  %%.t%d =%c sub 0, %%.t%d\n", r, cgqbetype(type), r);
  return (r);
}

// Invert a temporary's value
int cginvert(int r, int type) {
  fprintf(Outfile, "  %%.t%d =%c xor %%.t%d, -1\n", r, cgqbetype(type), r);
  return (r);
}

// Logically negate a temporary's value
int cglognot(int r, int type) {
  char q = cgqbetype(type);
  fprintf(Outfile, "  %%.t%d =%c ceq%c %%.t%d, 0\n", r, q, q, r);
  return (r);
}

// Load a boolean value (only 0 or 1)
// into the given temporary
void cgloadboolean(int r, int val, int type) {
  fprintf(Outfile, "  %%.t%d =%c copy %d\n", r, cgqbetype(type), val);
}

// Convert an integer value to a boolean value. Jump if
// it's an IF, WHILE, LOGAND or LOGOR operation
int cgboolean(int r, int op, int label, int type) {
  // Get a label for the next instruction
  int label2 = genlabel();

  // Get a new temporary for the comparison
  int r2 = cgalloctemp();

  // Convert temporary to boolean value
  fprintf(Outfile, "  %%.t%d =l cne%c %%.t%d, 0\n", r2, cgqbetype(type), r);

  switch (op) {
    case A_IF:
    case A_WHILE:
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
    fprintf(Outfile, "  %%.t%d =%c call $%s(", outr, cgqbetype(sym->type),
	    sym->name);

  // Output the list of arguments
  for (i = numargs - 1; i >= 0; i--) {
    fprintf(Outfile, "%c %%.t%d, ", cgqbetype(typelist[i]), arglist[i]);
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
  char q = cgqbetype(sym->type);
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
	    cgqbetype(sym->type), r, sym->name);
  } else {
    fprintf(Outfile, "  %%%s =%c copy %%.t%d\n",
	    sym->name, cgqbetype(sym->type), r);
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
  if (node->class == C_GLOBAL)
    fprintf(Outfile, "export ");
  if ((node->type == P_STRUCT) || (node->type == P_UNION))
    fprintf(Outfile, "data $%s = align 8 { ", node->name);
  else
    fprintf(Outfile, "data $%s = align %d { ", node->name, cgprimsize(type));

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

// Generate a global string and its label.
// Don't output the label if append is true.
void cgglobstr(int l, char *strvalue, int append) {
  char *cptr;
  if (!append)
    fprintf(Outfile, "data $L%d = { ", l);

  for (cptr = strvalue; *cptr; cptr++) {
    fprintf(Outfile, "b %d, ", *cptr);
  }
}

// NUL terminate a global string
void cgglobstrend(void) {
  fprintf(Outfile, " b 0 }\n");
}

// List of comparison instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *cmplist[] = { "ceq", "cne", "cslt", "csgt", "csle", "csge" };

// Compare two temporaries and set if true.
int cgcompare_and_set(int ASTop, int r1, int r2, int type) {
  int r3;
  char q = cgqbetype(type);

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
  fprintf(Outfile, "  jmp @L%d\n", l);
}

// List of inverted jump instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *invcmplist[] = { "cne", "ceq", "csge", "csle", "csgt", "cslt" };

// Compare two temporaries and jump if false.
int cgcompare_and_jump(int ASTop, int r1, int r2, int label, int type) {
  int label2;
  int r3;
  char q = cgqbetype(type);

  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in cgcompare_and_set()");

  // Get a label for the next instruction
  label2 = genlabel();

  // Get a new temporary for the comparison
  r3 = cgalloctemp();

  fprintf(Outfile, "  %%.t%d =%c %s%c %%.t%d, %%.t%d\n",
	  r3, q, invcmplist[ASTop - A_EQ], q, r1, r2);
  fprintf(Outfile, "  jnz %%.t%d, @L%d, @L%d\n", r3, label, label2);
  cglabel(label2);
  return (NOREG);
}

// Widen the value in the temporary from the old
// to the new type, and return a temporary with
// this new value
int cgwiden(int r, int oldtype, int newtype) {
  char oldq = cgqbetype(oldtype);
  char newq = cgqbetype(newtype);

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
void cgreturn(int reg, struct symtable *sym) {

  // Only return a value if we have a value to return
  if (reg != NOREG)
    fprintf(Outfile, "  %%.ret =%c copy %%.t%d\n", cgqbetype(sym->type), reg);

  cgjump(sym->st_endlabel);
}

// Generate code to load the address of an
// identifier. Return a new temporary
int cgaddress(struct symtable *sym) {
  int r = cgalloctemp();
  char qbeprefix = ((sym->class == C_GLOBAL) || (sym->class == C_STATIC) ||
		    (sym->class == C_EXTERN)) ? '$' : '%';

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

// Move value between temporaries
void cgmove(int r1, int r2, int type) {
  fprintf(Outfile, "  %%.t%d =%c copy %%.t%d\n", r2, cgqbetype(type), r1);
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
  char qnew;

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
  qnew = cgqbetype(newtype);
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
