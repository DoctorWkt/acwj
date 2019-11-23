#include "defs.h"
#include "data.h"
#include "decl.h"

// Code generator for x86-64
// Copyright (c) 2019 Warren Toomey, GPL3

// Flag to say which section were are outputting in to
enum { no_seg, text_seg, data_seg } currSeg = no_seg;

void cgtextseg() {
  if (currSeg != text_seg) {
    fputs("\t.text\n", Outfile);
    currSeg = text_seg;
  }
}

void cgdataseg() {
  if (currSeg != data_seg) {
    fputs("\t.data\n", Outfile);
    currSeg = data_seg;
  }
}

// Given a scalar type value, return the
// size of the type in bytes.
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
      return (offset);
    case P_INT:
    case P_LONG:
      break;
    default:
      fatald("Bad type in calc_aligned_offset:", type);
  }

  // Here we have an int or a long. Align it on a 4-byte offset
  // I put the generic code here so it can be reused elsewhere.
  alignment = 4;
  offset = (offset + direction * (alignment - 1)) & ~(alignment - 1);
  return (offset);
}

// Position of next local variable relative to stack base pointer.
// We store the offset as positive to make aligning the stack pointer easier
static int localOffset;
static int stackOffset;

// Create the position of a new local variable.
static int newlocaloffset(int type) {
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
static char *reglist[] =
  { "%r10", "%r11", "%r12", "%r13", "%r9", "%r8", "%rcx", "%rdx", "%rsi",
  "%rdi"
};

static char *breglist[] =
  { "%r10b", "%r11b", "%r12b", "%r13b", "%r9b", "%r8b", "%cl", "%dl", "%sil",
  "%dil"
};

static char *dreglist[] =
  { "%r10d", "%r11d", "%r12d", "%r13d", "%r9d", "%r8d", "%ecx", "%edx",
  "%esi", "%edi"
};


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
  cgtextseg();
  fprintf(Outfile,
	  "# internal switch(expr) routine\n"
	  "# %%rsi = switch table, %%rax = expr\n"
	  "# from SubC: http://www.t3x.org/subc/\n"
	  "\n"
	  "switch:\n"
	  "        pushq   %%rsi\n"
	  "        movq    %%rdx, %%rsi\n"
	  "        movq    %%rax, %%rbx\n"
	  "        cld\n"
	  "        lodsq\n"
	  "        movq    %%rax, %%rcx\n"
	  "next:\n"
	  "        lodsq\n"
	  "        movq    %%rax, %%rdx\n"
	  "        lodsq\n"
	  "        cmpq    %%rdx, %%rbx\n"
	  "        jnz     no\n"
	  "        popq    %%rsi\n"
	  "        jmp     *%%rax\n"
	  "no:\n"
	  "        loop    next\n"
	  "        lodsq\n"
	  "        popq    %%rsi\n" "        jmp     *%%rax\n" "\n");
}

// Nothing to do
void cgpostamble() {
}

// Print out a function preamble
void cgfuncpreamble(struct symtable *sym) {
  char *name = sym->name;
  struct symtable *parm, *locvar;
  int cnt;
  int paramOffset = 16;		// Any pushed params start at this stack offset
  int paramReg = FIRSTPARAMREG;	// Index to the first param register in above reg lists

  // Output in the text segment, reset local offset
  cgtextseg();
  localOffset = 0;

  // Output the function start, save the %rsp and %rsp
  fprintf(Outfile,
	  "\t.globl\t%s\n"
	  "\t.type\t%s, @function\n"
	  "%s:\n" "\tpushq\t%%rbp\n"
	  "\tmovq\t%%rsp, %%rbp\n", name, name, name);

  // Copy any in-register parameters to the stack, up to six of them
  // The remaining parameters are already on the stack
  for (parm = sym->member, cnt = 1; parm != NULL; parm = parm->next, cnt++) {
    if (cnt > 6) {
      parm->posn = paramOffset;
      paramOffset += 8;
    } else {
      parm->posn = newlocaloffset(parm->type);
      cgstorlocal(paramReg--, parm);
    }
  }

  // For the remainder, if they are a parameter then they are
  // already on the stack. If only a local, make a stack position.
  for (locvar = Loclhead; locvar != NULL; locvar = locvar->next) {
    locvar->posn = newlocaloffset(locvar->type);
  }

  // Align the stack pointer to be a multiple of 16
  // less than its previous value
  stackOffset = (localOffset + 15) & ~15;
  fprintf(Outfile, "\taddq\t$%d,%%rsp\n", -stackOffset);
}

// Print out a function postamble
void cgfuncpostamble(struct symtable *sym) {
  cglabel(sym->endlabel);
  fprintf(Outfile, "\taddq\t$%d,%%rsp\n", stackOffset);
  fputs("\tpopq	%rbp\n" "\tret\n", Outfile);
}

// Load an integer literal value into a register.
// Return the number of the register.
// For x86-64, we don't need to worry about the type.
int cgloadint(int value, int type) {
  // Get a new register
  int r = alloc_register();

  fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
  return (r);
}

// Load a value from a variable into a register.
// Return the number of the register. If the
// operation is pre- or post-increment/decrement,
// also perform this action.
int cgloadglob(struct symtable *sym, int op) {
  // Get a new register
  int r = alloc_register();

  if (cgprimsize(sym->type) == 8) {
    if (op == A_PREINC)
      fprintf(Outfile, "\tincq\t%s(%%rip)\n", sym->name);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecq\t%s(%%rip)\n", sym->name);
    fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", sym->name, reglist[r]);
    if (op == A_POSTINC)
      fprintf(Outfile, "\tincq\t%s(%%rip)\n", sym->name);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecq\t%s(%%rip)\n", sym->name);
  } else
    // Print out the code to initialise it
    switch (sym->type) {
      case P_CHAR:
	if (op == A_PREINC)
	  fprintf(Outfile, "\tincb\t%s(%%rip)\n", sym->name);
	if (op == A_PREDEC)
	  fprintf(Outfile, "\tdecb\t%s(%%rip)\n", sym->name);
	fprintf(Outfile, "\tmovzbq\t%s(%%rip), %s\n", sym->name, reglist[r]);
	if (op == A_POSTINC)
	  fprintf(Outfile, "\tincb\t%s(%%rip)\n", sym->name);
	if (op == A_POSTDEC)
	  fprintf(Outfile, "\tdecb\t%s(%%rip)\n", sym->name);
	break;
      case P_INT:
	if (op == A_PREINC)
	  fprintf(Outfile, "\tincl\t%s(%%rip)\n", sym->name);
	if (op == A_PREDEC)
	  fprintf(Outfile, "\tdecl\t%s(%%rip)\n", sym->name);
	fprintf(Outfile, "\tmovslq\t%s(%%rip), %s\n", sym->name, reglist[r]);
	if (op == A_POSTINC)
	  fprintf(Outfile, "\tincl\t%s(%%rip)\n", sym->name);
	if (op == A_POSTDEC)
	  fprintf(Outfile, "\tdecl\t%s(%%rip)\n", sym->name);
	break;
      default:
	fatald("Bad type in cgloadglob:", sym->type);
    }
  return (r);
}

// Load a value from a local variable into a register.
// Return the number of the register. If the
// operation is pre- or post-increment/decrement,
// also perform this action.
int cgloadlocal(struct symtable *sym, int op) {
  // Get a new register
  int r = alloc_register();

  // Print out the code to initialise it
  if (cgprimsize(sym->type) == 8) {
    if (op == A_PREINC)
      fprintf(Outfile, "\tincq\t%d(%%rbp)\n", sym->posn);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", sym->posn);
    fprintf(Outfile, "\tmovq\t%d(%%rbp), %s\n", sym->posn, reglist[r]);
    if (op == A_POSTINC)
      fprintf(Outfile, "\tincq\t%d(%%rbp)\n", sym->posn);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", sym->posn);
  } else
    switch (sym->type) {
      case P_CHAR:
	if (op == A_PREINC)
	  fprintf(Outfile, "\tincb\t%d(%%rbp)\n", sym->posn);
	if (op == A_PREDEC)
	  fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", sym->posn);
	fprintf(Outfile, "\tmovzbq\t%d(%%rbp), %s\n", sym->posn, reglist[r]);
	if (op == A_POSTINC)
	  fprintf(Outfile, "\tincb\t%d(%%rbp)\n", sym->posn);
	if (op == A_POSTDEC)
	  fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", sym->posn);
	break;
      case P_INT:
	if (op == A_PREINC)
	  fprintf(Outfile, "\tincl\t%d(%%rbp)\n", sym->posn);
	if (op == A_PREDEC)
	  fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", sym->posn);
	fprintf(Outfile, "\tmovslq\t%d(%%rbp), %s\n", sym->posn, reglist[r]);
	if (op == A_POSTINC)
	  fprintf(Outfile, "\tincl\t%d(%%rbp)\n", sym->posn);
	if (op == A_POSTDEC)
	  fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", sym->posn);
	break;
      default:
	fatald("Bad type in cgloadlocal:", sym->type);
    }
  return (r);
}

// Given the label number of a global string,
// load its address into a new register
int cgloadglobstr(int label) {
  // Get a new register
  int r = alloc_register();
  fprintf(Outfile, "\tleaq\tL%d(%%rip), %s\n", label, reglist[r]);
  return (r);
}

// Add two registers together and return
// the number of the register with the result
int cgadd(int r1, int r2) {
  fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return (r2);
}

// Subtract the second register from the first and
// return the number of the register with the result
int cgsub(int r1, int r2) {
  fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r2);
  return (r1);
}

// Multiply two registers together and return
// the number of the register with the result
int cgmul(int r1, int r2) {
  fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return (r2);
}

// Divide the first register by the second and
// return the number of the register with the result
int cgdiv(int r1, int r2) {
  fprintf(Outfile, "\tmovq\t%s,%%rax\n", reglist[r1]);
  fprintf(Outfile, "\tcqo\n");
  fprintf(Outfile, "\tidivq\t%s\n", reglist[r2]);
  fprintf(Outfile, "\tmovq\t%%rax,%s\n", reglist[r1]);
  free_register(r2);
  return (r1);
}

int cgand(int r1, int r2) {
  fprintf(Outfile, "\tandq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return (r2);
}

int cgor(int r1, int r2) {
  fprintf(Outfile, "\torq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return (r2);
}

int cgxor(int r1, int r2) {
  fprintf(Outfile, "\txorq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return (r2);
}

int cgshl(int r1, int r2) {
  fprintf(Outfile, "\tmovb\t%s, %%cl\n", breglist[r2]);
  fprintf(Outfile, "\tshlq\t%%cl, %s\n", reglist[r1]);
  free_register(r2);
  return (r1);
}

int cgshr(int r1, int r2) {
  fprintf(Outfile, "\tmovb\t%s, %%cl\n", breglist[r2]);
  fprintf(Outfile, "\tshrq\t%%cl, %s\n", reglist[r1]);
  free_register(r2);
  return (r1);
}

// Negate a register's value
int cgnegate(int r) {
  fprintf(Outfile, "\tnegq\t%s\n", reglist[r]);
  return (r);
}

// Invert a register's value
int cginvert(int r) {
  fprintf(Outfile, "\tnotq\t%s\n", reglist[r]);
  return (r);
}

// Logically negate a register's value
int cglognot(int r) {
  fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
  fprintf(Outfile, "\tsete\t%s\n", breglist[r]);
  fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
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
    fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
  }
  return (r);
}

// Call a function with the given symbol id
// Pop off any arguments pushed on the stack
// Return the register with the result
int cgcall(struct symtable *sym, int numargs) {
  // Get a new register
  int outr = alloc_register();
  // Call the function
  fprintf(Outfile, "\tcall\t%s@PLT\n", sym->name);
  // Remove any arguments pushed on the stack
  if (numargs > 6)
    fprintf(Outfile, "\taddq\t$%d, %%rsp\n", 8 * (numargs - 6));
  // and copy the return value into our register
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);
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
    fprintf(Outfile, "\tpushq\t%s\n", reglist[r]);
  } else {
    // Otherwise, copy the value into one of the six registers
    // used to hold parameter values
    fprintf(Outfile, "\tmovq\t%s, %s\n", reglist[r],
	    reglist[FIRSTPARAMREG - argposn + 1]);
  }
}

// Shift a register left by a constant
int cgshlconst(int r, int val) {
  fprintf(Outfile, "\tsalq\t$%d, %s\n", val, reglist[r]);
  return (r);
}

// Store a register's value into a variable
int cgstorglob(int r, struct symtable *sym) {

  if (cgprimsize(sym->type) == 8) {
    fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n", reglist[r], sym->name);
  } else
    switch (sym->type) {
      case P_CHAR:
	fprintf(Outfile, "\tmovb\t%s, %s(%%rip)\n", breglist[r], sym->name);
	break;
      case P_INT:
	fprintf(Outfile, "\tmovl\t%s, %s(%%rip)\n", dreglist[r], sym->name);
	break;
      default:
	fatald("Bad type in cgstorglob:", sym->type);
    }
  return (r);
}

// Store a register's value into a local variable
int cgstorlocal(int r, struct symtable *sym) {

  if (cgprimsize(sym->type) == 8) {
    fprintf(Outfile, "\tmovq\t%s, %d(%%rbp)\n", reglist[r], sym->posn);
  } else
    switch (sym->type) {
      case P_CHAR:
	fprintf(Outfile, "\tmovb\t%s, %d(%%rbp)\n", breglist[r], sym->posn);
	break;
      case P_INT:
	fprintf(Outfile, "\tmovl\t%s, %d(%%rbp)\n", dreglist[r], sym->posn);
	break;
      default:
	fatald("Bad type in cgstorlocal:", sym->type);
    }
  return (r);
}

// Generate a global symbol but not functions
void cgglobsym(struct symtable *node) {
  int size;

  if (node == NULL)
    return;
  if (node->stype == S_FUNCTION)
    return;

  // Get the size of the type
  size = typesize(node->type, node->ctype);

  // Generate the global identity and the label
  cgdataseg();
  fprintf(Outfile, "\t.globl\t%s\n", node->name);
  fprintf(Outfile, "%s:", node->name);

  // Generate the space for this type
  switch (size) {
    case 1:
      fprintf(Outfile, "\t.byte\t0\n");
      break;
    case 4:
      fprintf(Outfile, "\t.long\t0\n");
      break;
    case 8:
      fprintf(Outfile, "\t.quad\t0\n");
      break;
    default:
      for (int i = 0; i < size; i++)
	fprintf(Outfile, "\t.byte\t0\n");
  }
}

// Generate a global string and its start label
void cgglobstr(int l, char *strvalue) {
  char *cptr;
  cglabel(l);
  for (cptr = strvalue; *cptr; cptr++) {
    fprintf(Outfile, "\t.byte\t%d\n", *cptr);
  }
  fprintf(Outfile, "\t.byte\t0\n");
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

  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  fprintf(Outfile, "\t%s\t%s\n", cmplist[ASTop - A_EQ], breglist[r2]);
  fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r2], reglist[r2]);
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

  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
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
void cgreturn(int reg, struct symtable *sym) {
  // Generate code depending on the function's type
  switch (sym->type) {
    case P_CHAR:
      fprintf(Outfile, "\tmovzbl\t%s, %%eax\n", breglist[reg]);
      break;
    case P_INT:
      fprintf(Outfile, "\tmovl\t%s, %%eax\n", dreglist[reg]);
      break;
    case P_LONG:
      fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[reg]);
      break;
    default:
      fatald("Bad function type in cgreturn:", sym->type);
  }
  cgjump(sym->endlabel);
}


// Generate code to load the address of an
// identifier into a variable. Return a new register
int cgaddress(struct symtable *sym) {
  int r = alloc_register();

  if (sym->class == C_GLOBAL)
    fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", sym->name, reglist[r]);
  else
    fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", sym->posn, reglist[r]);
  return (r);
}

// Dereference a pointer to get the value it
// pointing at into the same register
int cgderef(int r, int type) {
  // Get the type that we are pointing to
  int newtype = value_at(type);
  // Now get the size of this type
  int size = cgprimsize(newtype);

  switch (size) {
    case 1:
      fprintf(Outfile, "\tmovzbq\t(%s), %s\n", reglist[r], reglist[r]);
      break;
    case 2:
      fprintf(Outfile, "\tmovslq\t(%s), %s\n", reglist[r], reglist[r]);
      break;
    case 4:
    case 8:
      fprintf(Outfile, "\tmovq\t(%s), %s\n", reglist[r], reglist[r]);
      break;
    default:
      fatald("Can't cgderef on type:", type);
  }
  return (r);
}

// Store through a dereferenced pointer
int cgstorderef(int r1, int r2, int type) {
  // Get the size of the type
  int size = cgprimsize(type);

  switch (size) {
    case 1:
      fprintf(Outfile, "\tmovb\t%s, (%s)\n", breglist[r1], reglist[r2]);
      break;
    case 2:
    case 4:
    case 8:
      fprintf(Outfile, "\tmovq\t%s, (%s)\n", reglist[r1], reglist[r2]);
      break;
    default:
      fatald("Can't cgstoderef on type:", type);
  }
  return (r1);
}

// Generate a switch jump table and the code to
// load the registers and call the switch() code
void cgswitch(int reg, int casecount, int toplabel,
	      int *caselabel, int *caseval, int defaultlabel) {
  int i, label;

  // Get a label for the switch table
  label = genlabel();
  cglabel(label);

  // Heuristic. If we have no cases, create one case
  // which points to the default case
  if (casecount == 0) {
    caseval[0] = 0;
    caselabel[0] = defaultlabel;
    casecount = 1;
  }
  // Generate the switch jump table.
  fprintf(Outfile, "\t.quad\t%d\n", casecount);
  for (i = 0; i < casecount; i++)
    fprintf(Outfile, "\t.quad\t%d, L%d\n", caseval[i], caselabel[i]);
  fprintf(Outfile, "\t.quad\tL%d\n", defaultlabel);

  // Load the specific registers
  cglabel(toplabel);
  fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[reg]);
  fprintf(Outfile, "\tleaq\tL%d(%%rip), %%rdx\n", label);
  fprintf(Outfile, "\tjmp\tswitch\n");
}
