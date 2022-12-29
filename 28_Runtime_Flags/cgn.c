#include "defs.h"
#include "data.h"
#include "decl.h"

// Code generator for x86-64
// Copyright (c) 2019 Warren Toomey, GPL3

// Flag to say which section were are outputting in to
enum { no_seg, text_seg, data_seg } currSeg = no_seg;

void cgtextseg() {
  if (currSeg != text_seg) {
    fputs("\tsection .text\n", Outfile);
    currSeg = text_seg;
  }
}

void cgdataseg() {
  if (currSeg != data_seg) {
    fputs("\tsection .data\n", Outfile);
    currSeg = data_seg;
  }
}

// Position of next local variable relative to stack base pointer.
// We store the offset as positive to make aligning the stack pointer easier
static int localOffset;
static int stackOffset;

// Create the position of a new local variable.
int newlocaloffset(int type) {
  // Decrement the offset by a minimum of 4 bytes
  // and allocate on the stack
  localOffset += (cgprimsize(type) > 4) ? cgprimsize(type) : 4;
  return (-localOffset);
}

// List of available registers and their names.
// We need a list of byte and doubleword registers, too
// The list also includes the registers used to
// hold function parameters
#define NUMFREEREGS 4
#define FIRSTPARAMREG 9		// Position of first parameter register
static int freereg[NUMFREEREGS];
static char *reglist[]  = { "r10",  "r11", "r12", "r13", "r9", "r8", "rcx", "rdx", "rsi",
"rdi"  };
static char *breglist[]  = { "r10b",  "r11b", "r12b", "r13b", "r9b", "r8b", "cl", "dl", "sil",
"dil"  };
static char *dreglist[]  = { "r10d",  "r11d", "r12d", "r13d", "r9d", "r8d", "ecx", "edx",
"esi", "edi"  };

// Set all registers as available
void freeall_registers(void) {
  freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

// Allocate a free register. Return the number of
// the register. Die if no available registers.
static int alloc_register(void) {
  for (int i = 0; i < NUMFREEREGS; i++) {
    if (freereg[i]) {
      freereg[i] = 0;
      return (i);
    }
  }
  fatal("Out of registers");
  return (NOREG);		// Keep -Wall happy
}

// Return a register to the list of available registers.
// Check to see if it's not already there.
static void free_register(int reg) {
  if (freereg[reg] != 0)
    fatald("Error trying to free register", reg);
  freereg[reg] = 1;
}

// Print out the assembly preamble
void cgpreamble() {
  freeall_registers();
  fputs("\textern\tprintint\n", Outfile);
  fputs("\textern\tprintchar\n", Outfile);
  fputs("\textern\topen\n", Outfile);
  fputs("\textern\tclose\n", Outfile);
  fputs("\textern\tread\n", Outfile);
  fputs("\textern\twrite\n", Outfile);
  fputs("\textern\tprintf\n", Outfile);
}

// Nothing to do
void cgpostamble() {
}

// Print out a function preamble
void cgfuncpreamble(int id) {
  char *name = Symtable[id].name;
  int i;
  int paramOffset = 16;		// Any pushed params start at this stack offset
  int paramReg = FIRSTPARAMREG;	// Index to the first param register in above reg lists

  // Output in the text segment, reset local offset
  cgtextseg();
  localOffset = 0;

  // Output the function start, save the rsp and rbp
  fprintf(Outfile,
	  "\tglobal\t%s\n"
	  "%s:\n" "\tpush\trbp\n"
	  "\tmov\trbp, rsp\n", name, name);

  // Copy any in-register parameters to the stack
  // Stop after no more than six parameter registers
  for (i = NSYMBOLS - 1; i > Locls; i--) {
    if (Symtable[i].class != C_PARAM)
      break;
    if (i < NSYMBOLS - 6)
      break;
    Symtable[i].posn = newlocaloffset(Symtable[i].type);
    cgstorlocal(paramReg--, i);
  }

  // For the remainder, if they are a parameter then they are
  // already on the stack. If only a local, make a stack position.
  for (; i > Locls; i--) {
    if (Symtable[i].class == C_PARAM) {
      Symtable[i].posn = paramOffset;
      paramOffset += 8;
    } else {
      Symtable[i].posn = newlocaloffset(Symtable[i].type);
    }
  }

  // Align the stack pointer to be a multiple of 16
  // less than its previous value
  stackOffset = (localOffset + 15) & ~15;
  fprintf(Outfile, "\tadd\trsp, %d\n", -stackOffset);
}

// Print out a function postamble
void cgfuncpostamble(int id) {
  cglabel(Symtable[id].endlabel);
  fprintf(Outfile, "\tadd\trsp, %d\n", stackOffset);
  fputs("\tpop	rbp\n" "\tret\n", Outfile);
}

// Load an integer literal value into a register.
// Return the number of the register.
// For x86-64, we don't need to worry about the type.
int cgloadint(int value, int type) {
  // Get a new register
  int r = alloc_register();

  fprintf(Outfile, "\tmov\t%s, %d\n", reglist[r], value);
  return (r);
}

// Load a value from a variable into a register.
// Return the number of the register. If the
// operation is pre- or post-increment/decrement,
// also perform this action.
int cgloadglob(int id, int op) {
  // Get a new register
  int r = alloc_register();

  // Print out the code to initialise it
  switch (Symtable[id].type) {
    case P_CHAR:
      if (op == A_PREINC)
        fprintf(Outfile, "\tinc\tbyte [%s]\n", Symtable[id].name);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdec\tbyte [%s]\n", Symtable[id].name);
      fprintf(Outfile, "\tmovzx\t%s, byte [%s]\n", reglist[r], 
              Symtable[id].name);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tinc\tbyte [%s]\n", Symtable[id].name);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdec\tbyte [%s]\n", Symtable[id].name);
      break;
    case P_INT:
      if (op == A_PREINC)
        fprintf(Outfile, "\tinc\tdword [%s]\n", Symtable[id].name);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdec\tdword [%s]\n", Symtable[id].name);
      fprintf(Outfile, "\tmovsx\t%s, word [%s]\n", dreglist[r], 
              Symtable[id].name);
      fprintf(Outfile, "\tmovsxd\t%s, %s\n", reglist[r], dreglist[r]);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tinc\tdword [%s]\n", Symtable[id].name);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdec\tdword [%s]\n", Symtable[id].name);
      break;
    case P_LONG:
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
      if (op == A_PREINC)
        fprintf(Outfile, "\tinc\tqword [%s]\n", Symtable[id].name);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdec\tqword [%s]\n", Symtable[id].name);
      fprintf(Outfile, "\tmov\t%s, [%s]\n", reglist[r], Symtable[id].name);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tinc\tqword [%s]\n", Symtable[id].name);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdec\tqword [%s]\n", Symtable[id].name);
      break;
    default:
      fatald("Bad type in cgloadglob:", Symtable[id].type);
  }
  return (r);
}

// Load a value from a local variable into a register.
// Return the number of the register. If the
// operation is pre- or post-increment/decrement,
// also perform this action.
int cgloadlocal(int id, int op) {
  // Get a new register
  int r = alloc_register();

  // Print out the code to initialise it
  switch (Symtable[id].type) {
    case P_CHAR:
      if (op == A_PREINC)
        fprintf(Outfile, "\tinc\tbyte\t[rbp+%d]\n", Symtable[id].posn);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdec\tbyte\t[rbp+%d]\n", Symtable[id].posn);
      fprintf(Outfile, "\tmovzx\t%s, byte [rbp+%d]\n", reglist[r], 
              Symtable[id].posn);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tinc\tbyte\t[rbp+%d]\n", Symtable[id].posn);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdec\tbyte\t[rbp+%d]\n", Symtable[id].posn);
      break;
    case P_INT:
      if (op == A_PREINC)
        fprintf(Outfile, "\tinc\tdword\t[rbp+%d]\n", Symtable[id].posn);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdec\tdword\t[rbp+%d]\n", Symtable[id].posn);
      fprintf(Outfile, "\tmovsx\t%s, word [rbp+%d]\n", reglist[r], 
              Symtable[id].posn);
      fprintf(Outfile, "\tmovsxd\t%s, %s\n", reglist[r], dreglist[r]);
      if (op == A_POSTINC)
      if (op == A_POSTINC)
        fprintf(Outfile, "\tinc\tdword\t[rbp+%d]\n", Symtable[id].posn);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdec\tdword\t[rbp+%d]\n", Symtable[id].posn);
      break;
    case P_LONG:
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
      if (op == A_PREINC)
        fprintf(Outfile, "\tinc\tqword\t[rbp+%d]\n", Symtable[id].posn);
      if (op == A_PREDEC)
        fprintf(Outfile, "\tdec\tqword\t[rbp+%d]\n", Symtable[id].posn);
      fprintf(Outfile, "\tmov\t%s, [rbp+%d]\n", reglist[r],
              Symtable[id].posn);
      if (op == A_POSTINC)
        fprintf(Outfile, "\tinc\tqword\t[rbp+%d]\n", Symtable[id].posn);
      if (op == A_POSTDEC)
        fprintf(Outfile, "\tdec\tqword\t[rbp+%d]\n", Symtable[id].posn);
      break;
    default:
      fatald("Bad type in cgloadlocal:", Symtable[id].type);
  }
  return (r);
}

// Given the label number of a global string,
// load its address into a new register
int cgloadglobstr(int id) {
  // Get a new register
  int r = alloc_register();
  fprintf(Outfile, "\tmov\t%s, L%d\n", reglist[r], id);
  return (r);
}

// Add two registers together and return
// the number of the register with the result
int cgadd(int r1, int r2) {
  fprintf(Outfile, "\tadd\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r1);
  return (r2);
}

// Subtract the second register from the first and
// return the number of the register with the result
int cgsub(int r1, int r2) {
  fprintf(Outfile, "\tsub\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r2);
  return (r1);
}

// Multiply two registers together and return
// the number of the register with the result
int cgmul(int r1, int r2) {
  fprintf(Outfile, "\timul\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r1);
  return (r2);
}

// Divide the first register by the second and
// return the number of the register with the result
int cgdiv(int r1, int r2) {
  fprintf(Outfile, "\tmov\trax, %s\n", reglist[r1]);
  fprintf(Outfile, "\tcqo\n");
  fprintf(Outfile, "\tidiv\t%s\n", reglist[r2]);
  fprintf(Outfile, "\tmov\t%s, rax\n", reglist[r1]);
  free_register(r2);
  return (r1);
}

int cgand(int r1, int r2) {
  fprintf(Outfile, "\tand\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r1);
  return (r2);
}

int cgor(int r1, int r2) {
  fprintf(Outfile, "\tor\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r1);
  return (r2);
}

int cgxor(int r1, int r2) {
  fprintf(Outfile, "\txor\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r1);
  return (r2);
}

int cgshl(int r1, int r2) {
  fprintf(Outfile, "\tmov\tcl, %s\n", breglist[r2]);
  fprintf(Outfile, "\tshl\t%s, cl\n", reglist[r1]);
  free_register(r2);
  return (r1);
}

int cgshr(int r1, int r2) {
  fprintf(Outfile, "\tmov\tcl, %s\n", breglist[r2]);
  fprintf(Outfile, "\tshr\t%s, cl\n", reglist[r1]);
  free_register(r2);
  return (r1);
}

// Negate a register's value
int cgnegate(int r) {
  fprintf(Outfile, "\tneg\t%s\n", reglist[r]);
  return (r);
}

// Invert a register's value
int cginvert(int r) {
  fprintf(Outfile, "\tnot\t%s\n", reglist[r]);
  return (r);
}

// Logically negate a register's value
int cglognot(int r) {
  fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
  fprintf(Outfile, "\tsete\t%s\n", breglist[r]);
  fprintf(Outfile, "\tmovzx\t%s, %s\n", reglist[r], breglist[r]);
  return (r);
}

// Convert an integer value to a boolean value. Jump if
// it's an IF or WHILE operation
int cgboolean(int r, int op, int label) {
  fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
  if (op == A_IF || op == A_WHILE)
    fprintf(Outfile, "\tje\tL%d\n", label);
  else {
    fprintf(Outfile, "\tsetnz\t%s\n", breglist[r]);
    fprintf(Outfile, "\tmovzx\t%s, byte %s\n", reglist[r], breglist[r]);
  }
  return (r);
}

// Call a function with the given symbol id
// Pop off any arguments pushed on the stack
// Return the register with the result
int cgcall(int id, int numargs) {
  // Get a new register
  int outr = alloc_register();
  // Call the function
  fprintf(Outfile, "\tcall\t%s\n", Symtable[id].name);
  // Remove any arguments pushed on the stack
  if (numargs > 6) 
    fprintf(Outfile, "\tadd\trsp, %d\n", 8 * (numargs - 6));
  // and copy the return value into our register
  fprintf(Outfile, "\tmov\t%s, rax\n", reglist[outr]);
  return (outr);
}

// Given a register with an argument value,
// copy this argument into the argposn'th
// parameter in preparation for a future function
// call. Note that argposn is 1, 2, 3, 4, ..., never zero.
void cgcopyarg(int r, int argposn) {

  // If this is above the sixth argument, simply push the
  // register on the stack. We rely on being called with
  // successive arguments in the correct order for x86-64
  if (argposn > 6) {
    fprintf(Outfile, "\tpush\t%s\n", reglist[r]);
  } else {
    // Otherwise, copy the value into one of the six registers
    // used to hold parameter values
    fprintf(Outfile, "\tmov\t%s, %s\n", 
	    reglist[FIRSTPARAMREG - argposn + 1], reglist[r]);
  }
}

// Shift a register left by a constant
int cgshlconst(int r, int val) {
  fprintf(Outfile, "\tsal\t%s, %d\n", reglist[r], val);
  return (r);
}

// Store a register's value into a variable
int cgstorglob(int r, int id) {
  switch (Symtable[id].type) {
    case P_CHAR:
      fprintf(Outfile, "\tmov\t[%s], %s\n", Symtable[id].name, breglist[r]);
      break;
    case P_INT:
      fprintf(Outfile, "\tmov\t[%s], %s\n", Symtable[id].name, dreglist[r]);
      break;
    case P_LONG:
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
      fprintf(Outfile, "\tmov\t[%s], %s\n", Symtable[id].name, reglist[r]);
      break;
    default:
      fatald("Bad type in cgloadglob:", Symtable[id].type);
  }
  return (r);
}

// Store a register's value into a local variable
int cgstorlocal(int r, int id) {
  switch (Symtable[id].type) {
    case P_CHAR:
      fprintf(Outfile, "\tmov\tbyte\t[rbp+%d], %s\n", Symtable[id].posn,
              breglist[r]);
      break;
    case P_INT:
      fprintf(Outfile, "\tmov\tdword\t[rbp+%d], %s\n", Symtable[id].posn,
              dreglist[r]);
      break;
    case P_LONG:
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
      fprintf(Outfile, "\tmov\tqword\t[rbp+%d], %s\n", Symtable[id].posn,
              reglist[r]);
      break;
    default:
      fatald("Bad type in cgstorlocal:", Symtable[id].type);
  }
  return (r);
}

// Array of type sizes in P_XXX order.
// 0 means no size.
static int psize[] = { 0, 0, 1, 4, 8, 8, 8, 8, 8 };

// Given a P_XXX type value, return the
// size of a primitive type in bytes.
int cgprimsize(int type) {
  // Check the type is valid
  if (type < P_NONE || type > P_LONGPTR)
    fatal("Bad type in cgprimsize()");
  return (psize[type]);
}

// Generate a global symbol but not functions
void cgglobsym(int id) {
  int typesize;
  if (Symtable[id].stype == S_FUNCTION)
    return;
  // Get the size of the type
  typesize = cgprimsize(Symtable[id].type);

  // Generate the global identity and the label
  cgdataseg();
  fprintf(Outfile, "\tsection\t.data\n" "\tglobal\t%s\n", Symtable[id].name);
  fprintf(Outfile, "%s:", Symtable[id].name);

  // Generate the space
  // original version
  for (int i = 0; i < Symtable[id].size; i++) {
    switch(typesize) {
      case 1:
        fprintf(Outfile, "\tdb\t0\n");
        break;
      case 4:
        fprintf(Outfile, "\tdd\t0\n");
        break;
      case 8:
        fprintf(Outfile, "\tdq\t0\n");
        break;
      default:
        fatald("Unknown typesize in cgglobsym: ", typesize);
    }
  }

  /* compact version using times instead of loop
  switch(typesize) {
    case 1:
      fprintf(Outfile, "\ttimes\t%d\tdb\t0\n", Symtable[id].size);
      break;
    case 4:
      fprintf(Outfile, "\ttimes\t%d\tdd\t0\n", Symtable[id].size);
      break;
    case 8:
      fprintf(Outfile, "\ttimes\t%d\tdq\t0\n", Symtable[id].size);
      break;
    default:
      fatald("Unknown typesize in cgglobsym: ", typesize);
  }
  */
}

// Generate a global string and its start label
void cgglobstr(int l, char *strvalue) {
  char *cptr;
  cglabel(l);
  for (cptr = strvalue; *cptr; cptr++) {
    fprintf(Outfile, "\tdb\t%d\n", *cptr);
  }
  fprintf(Outfile, "\tdb\t0\n");

  /* put string in readable format on a single line
  // probably went overboard with error checking
  int comma = 0, quote = 0, start = 1;
  fprintf(Outfile, "\tdb\t");
  for (cptr=strvalue; *cptr; cptr++) {
    if ( ! isprint(*cptr) )
      if (comma || start) {
        fprintf(Outfile, "%d, ", *cptr);
        start = 0;
        comma = 1;
      }
      else if (quote) {
        fprintf(Outfile, "\', %d, ", *cptr);
        comma = 1;
        quote = 0;
      }
      else {
        fprintf(Outfile, "%d, ", *cptr);
        comma = 1;
        quote = 0;
      }
    else
      if (start || comma) {
        fprintf(Outfile, "\'%c", *cptr);
        start = comma = 0;
        quote = 1;
      }
      else {
        fprintf(Outfile, "%c", *cptr);
        comma = 0;
        quote = 1;
      }
  }
  if (comma || start)
    fprintf(Outfile, "0\n");
  else
    fprintf(Outfile, "\', 0\n");
  */
}

// List of comparison instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *cmplist[] =
  { "sete", "setne", "setl", "setg", "setle", "setge" };

// Compare two registers and set if true.
int cgcompare_and_set(int ASTop, int r1, int r2) {

  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in cgcompare_and_set()");

  fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
  fprintf(Outfile, "\t%s\t%s\n", cmplist[ASTop - A_EQ], breglist[r2]);
  fprintf(Outfile, "\tmovzx\t%s, %s\n", reglist[r2], breglist[r2]);
  free_register(r1);
  return (r2);
}

// Generate a label
void cglabel(int l) {
  fprintf(Outfile, "L%d:\n", l);
}

// Generate a jump to a label
void cgjump(int l) {
  fprintf(Outfile, "\tjmp\tL%d\n", l);
}

// List of inverted jump instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *invcmplist[] = { "jne", "je", "jge", "jle", "jg", "jl" };

// Compare two registers and jump if false.
int cgcompare_and_jump(int ASTop, int r1, int r2, int label) {

  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in cgcompare_and_set()");

  fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
  fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
  freeall_registers();
  return (NOREG);
}

// Widen the value in the register from the old
// to the new type, and return a register with
// this new value
int cgwiden(int r, int oldtype, int newtype) {
  // Nothing to do
  return (r);
}

// Generate code to return a value from a function
void cgreturn(int reg, int id) {
  // Generate code depending on the function's type
  switch (Symtable[id].type) {
    case P_CHAR:
      fprintf(Outfile, "\tmovzx\teax, %s\n", breglist[reg]);
      break;
    case P_INT:
      fprintf(Outfile, "\tmov\teax, %s\n", dreglist[reg]);
      break;
    case P_LONG:
      fprintf(Outfile, "\tmov\trax, %s\n", reglist[reg]);
      break;
    default:
      fatald("Bad function type in cgreturn:", Symtable[id].type);
  }
  cgjump(Symtable[id].endlabel);
}

// Generate code to load the address of an
// identifier into a variable. Return a new register
int cgaddress(int id) {
  int r = alloc_register();

  if (Symtable[id].class == C_GLOBAL)
    fprintf(Outfile, "\tmov\t%s, %s\n", reglist[r], Symtable[id].name);
  else
    fprintf(Outfile, "\tlea\t%s, [rbp+%d]\n", reglist[r],
            Symtable[id].posn);
  return (r);
}

// Dereference a pointer to get the value it
// pointing at into the same register
int cgderef(int r, int type) {
  switch (type) {
    case P_CHARPTR:
      fprintf(Outfile, "\tmovzx\t%s, byte [%s]\n", reglist[r], reglist[r]);
      break;
    case P_INTPTR:
      fprintf(Outfile, "\tmovsx\t%s, dword [%s]\n", reglist[r], reglist[r]);
      break;
    case P_LONGPTR:
      fprintf(Outfile, "\tmov\t%s, [%s]\n", reglist[r], reglist[r]);
      break;
    default:
      fatald("Can't cgderef on type:", type);
  }
  return (r);
}

// Store through a dereferenced pointer
int cgstorderef(int r1, int r2, int type) {
  switch (type) {
    case P_CHAR:
      fprintf(Outfile, "\tmov\t[%s], byte %s\n", reglist[r2], breglist[r1]);
      break;
    case P_INT:
      fprintf(Outfile, "\tmov\t[%s], %s\n", reglist[r2], reglist[r1]);
      break;
    case P_LONG:
      fprintf(Outfile, "\tmov\t[%s], %s\n", reglist[r2], reglist[r1]);
      break;
    default:
      fatald("Can't cgstoderef on type:", type);
  }
  return (r1);
}
