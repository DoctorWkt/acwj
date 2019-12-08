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
    if (!ptrtype(type))
      fatald("Bad type in cg_align:", type);
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
static int newlocaloffset(int size) {
  // Decrement the offset by a minimum of 4 bytes
  // and allocate on the stack
  localOffset += (size > 4) ? size : 4;
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

// Push and pop a register on/off the stack
static void pushreg(int r) {
  fprintf(Outfile, "\tpushq\t%s\n", reglist[r]);
}

static void popreg(int r) {
  fprintf(Outfile, "\tpopq\t%s\n", reglist[r]);
}


// Set all registers as available.
// But if reg is positive, don't free that one.
void freeall_registers(int keepreg) {
  int i;
  // fprintf(Outfile, "# freeing all registers\n");
  for (i = 0; i < NUMFREEREGS; i++)
    if (i != keepreg)
      freereg[i] = 1;
}

// When we need to spill a register, we choose
// the following register and then cycle through
// the remaining registers. The spillreg increments
// continually, so we need to take a modulo NUMFREEREGS
// on it.
static int spillreg = 0;

// Allocate a free register. Return the number of
// the register. Die if no available registers.
int alloc_register(void) {
  int reg;

  for (reg = 0; reg < NUMFREEREGS; reg++) {
    if (freereg[reg]) {
      freereg[reg] = 0;
      // fprintf(Outfile, "# allocated register %s\n", reglist[reg]);
      return (reg);
    }
  }

  // We have no registers, so we must spill one
  reg = (spillreg % NUMFREEREGS);
  spillreg++;
  // fprintf(Outfile, "# spilling reg %s\n", reglist[reg]);
  pushreg(reg);
  return (reg);
}

// Return a register to the list of available registers.
// Check to see if it's not already there.
static void free_register(int reg) {
  if (freereg[reg] != 0) {
    // fprintf(Outfile, "# error trying to free register %s\n", reglist[reg]);
    fatald("Error trying to free register", reg);
  }
  // If this was a spilled register, get it back
  if (spillreg > 0) {
    spillreg--;
    reg = (spillreg % NUMFREEREGS);
    // fprintf(Outfile, "# unspilling reg %s\n", reglist[reg]);
    popreg(reg);
  } else {
    // fprintf(Outfile, "# freeing reg %s\n", reglist[reg]);
    freereg[reg] = 1;
  }
}

// Spill all registers on the stack
void spill_all_regs(void) {
  int i;

  for (i = 0; i < NUMFREEREGS; i++)
    pushreg(i);
}

// Unspill all registers from the stack
static void unspill_all_regs(void) {
  int i;

  for (i = NUMFREEREGS - 1; i >= 0; i--)
    popreg(i);
}

// Print out the assembly preamble
void cgpreamble(char *filename) {
  freeall_registers(NOREG);
  cgtextseg();
  fprintf(Outfile, "\t.file 1 ");
  fputc('"', Outfile);
  fprintf(Outfile, "%s", filename);
  fputc('"', Outfile);
  fputc('\n', Outfile);
  fprintf(Outfile,
	  "# internal switch(expr) routine\n"
	  "# %%rsi = switch table, %%rax = expr\n"
	  "# from SubC: http://www.t3x.org/subc/\n"
	  "\n"
	  "__switch:\n"
	  "        pushq   %%rsi\n"
	  "        movq    %%rdx, %%rsi\n"
	  "        movq    %%rax, %%rbx\n"
	  "        cld\n"
	  "        lodsq\n"
	  "        movq    %%rax, %%rcx\n"
	  "__next:\n"
	  "        lodsq\n"
	  "        movq    %%rax, %%rdx\n"
	  "        lodsq\n"
	  "        cmpq    %%rdx, %%rbx\n"
	  "        jnz     __no\n"
	  "        popq    %%rsi\n"
	  "        jmp     *%%rax\n"
	  "__no:\n"
	  "        loop    __next\n"
	  "        lodsq\n"
	  "        popq    %%rsi\n" "        jmp     *%%rax\n\n");
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
  if (sym->class == C_GLOBAL)
    fprintf(Outfile, "\t.globl\t%s\n" "\t.type\t%s, @function\n", name, name);
  fprintf(Outfile, "%s:\n" "\tpushq\t%%rbp\n" "\tmovq\t%%rsp, %%rbp\n", name);

  // Copy any in-register parameters to the stack, up to six of them
  // The remaining parameters are already on the stack
  for (parm = sym->member, cnt = 1; parm != NULL; parm = parm->next, cnt++) {
    if (cnt > 6) {
      parm->st_posn = paramOffset;
      paramOffset += 8;
    } else {
      parm->st_posn = newlocaloffset(parm->size);
      cgstorlocal(paramReg--, parm);
    }
  }

  // For the remainder, if they are a parameter then they are
  // already on the stack. If only a local, make a stack position.
  for (locvar = Loclhead; locvar != NULL; locvar = locvar->next) {
    locvar->st_posn = newlocaloffset(locvar->size);
  }

  // Align the stack pointer to be a multiple of 16
  // less than its previous value
  stackOffset = (localOffset + 15) & ~15;
  fprintf(Outfile, "\taddq\t$%d,%%rsp\n", -stackOffset);
}

// Print out a function postamble
void cgfuncpostamble(struct symtable *sym) {
  cglabel(sym->st_endlabel);
  fprintf(Outfile, "\taddq\t$%d,%%rsp\n", stackOffset);
  fputs("\tpopq	%rbp\n" "\tret\n", Outfile);
  freeall_registers(NOREG);
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
int cgloadvar(struct symtable *sym, int op) {
  int r, postreg, offset = 1;

  // Get a new register
  r = alloc_register();

  // If the symbol is a pointer, use the size
  // of the type that it points to as any
  // increment or decrement. If not, it's one.
  if (ptrtype(sym->type))
    offset = typesize(value_at(sym->type), sym->ctype);

  // Negate the offset for decrements
  if (op == A_PREDEC || op == A_POSTDEC)
    offset = -offset;

  // If we have a pre-operation
  if (op == A_PREINC || op == A_PREDEC) {
    // Load the symbol's address
    if (sym->class == C_LOCAL || sym->class == C_PARAM)
      fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", sym->st_posn, reglist[r]);
    else
      fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", sym->name, reglist[r]);

    // and change the value at that address
    switch (sym->size) {
    case 1:
      fprintf(Outfile, "\taddb\t$%d,(%s)\n", offset, reglist[r]);
      break;
    case 4:
      fprintf(Outfile, "\taddl\t$%d,(%s)\n", offset, reglist[r]);
      break;
    case 8:
      fprintf(Outfile, "\taddq\t$%d,(%s)\n", offset, reglist[r]);
      break;
    }
  }
  // Now load the output register with the value
  if (sym->class == C_LOCAL || sym->class == C_PARAM) {
    switch (sym->size) {
    case 1:
      fprintf(Outfile, "\tmovzbq\t%d(%%rbp), %s\n", sym->st_posn, reglist[r]);
      break;
    case 4:
      fprintf(Outfile, "\tmovslq\t%d(%%rbp), %s\n", sym->st_posn, reglist[r]);
      break;
    case 8:
      fprintf(Outfile, "\tmovq\t%d(%%rbp), %s\n", sym->st_posn, reglist[r]);
    }
  } else {
    switch (sym->size) {
    case 1:
      fprintf(Outfile, "\tmovzbq\t%s(%%rip), %s\n", sym->name, reglist[r]);
      break;
    case 4:
      fprintf(Outfile, "\tmovslq\t%s(%%rip), %s\n", sym->name, reglist[r]);
      break;
    case 8:
      fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", sym->name, reglist[r]);
    }
  }

  // If we have a post-operation, get a new register
  if (op == A_POSTINC || op == A_POSTDEC) {
    postreg = alloc_register();

    // Load the symbol's address
    if (sym->class == C_LOCAL || sym->class == C_PARAM)
      fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", sym->st_posn,
	      reglist[postreg]);
    else
      fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", sym->name,
	      reglist[postreg]);
    // and change the value at that address

    switch (sym->size) {
    case 1:
      fprintf(Outfile, "\taddb\t$%d,(%s)\n", offset, reglist[postreg]);
      break;
    case 4:
      fprintf(Outfile, "\taddl\t$%d,(%s)\n", offset, reglist[postreg]);
      break;
    case 8:
      fprintf(Outfile, "\taddq\t$%d,(%s)\n", offset, reglist[postreg]);
      break;
    }
    // and free the register
    free_register(postreg);
  }
  // Return the register with the value
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
  fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r2);
  return (r1);
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
  fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r2);
  return (r1);
}

// Divide or modulo the first register by the second and
// return the number of the register with the result
int cgdivmod(int r1, int r2, int op) {
  fprintf(Outfile, "\tmovq\t%s,%%rax\n", reglist[r1]);
  fprintf(Outfile, "\tcqo\n");
  fprintf(Outfile, "\tidivq\t%s\n", reglist[r2]);
  if (op == A_DIVIDE)
    fprintf(Outfile, "\tmovq\t%%rax,%s\n", reglist[r1]);
  else
    fprintf(Outfile, "\tmovq\t%%rdx,%s\n", reglist[r1]);
  free_register(r2);
  return (r1);
}

int cgand(int r1, int r2) {
  fprintf(Outfile, "\tandq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r2);
  return (r1);
}

int cgor(int r1, int r2) {
  fprintf(Outfile, "\torq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r2);
  return (r1);
}

int cgxor(int r1, int r2) {
  fprintf(Outfile, "\txorq\t%s, %s\n", reglist[r2], reglist[r1]);
  free_register(r2);
  return (r1);
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

// Load a boolean value (only 0 or 1)
// into the given register
void cgloadboolean(int r, int val) {
  fprintf(Outfile, "\tmovq\t$%d, %s\n", val, reglist[r]);
}


// Convert an integer value to a boolean value. Jump if
// it's an IF, WHILE, LOGAND or LOGOR operation
int cgboolean(int r, int op, int label) {
  fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
  switch (op) {
  case A_IF:
  case A_WHILE:
  case A_LOGAND:
    fprintf(Outfile, "\tje\tL%d\n", label);
    break;
  case A_LOGOR:
    fprintf(Outfile, "\tjne\tL%d\n", label);
    break;
  default:
    fprintf(Outfile, "\tsetnz\t%s\n", breglist[r]);
    fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
  }
  return (r);
}

// Call a function with the given symbol id
// Pop off any arguments pushed on the stack
// Return the register with the result
int cgcall(struct symtable *sym, int numargs) {
  int outr;

  // Call the function
  fprintf(Outfile, "\tcall\t%s@PLT\n", sym->name);

  // Remove any arguments pushed on the stack
  if (numargs > 6)
    fprintf(Outfile, "\taddq\t$%d, %%rsp\n", 8 * (numargs - 6));

  // Unspill all the registers
  unspill_all_regs();

  // Get a new register and copy the return value into it
  outr = alloc_register();
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
  free_register(r);
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
    fprintf(Outfile, "\tmovq\t%s, %d(%%rbp)\n", reglist[r], sym->st_posn);
  } else
    switch (sym->type) {
    case P_CHAR:
      fprintf(Outfile, "\tmovb\t%s, %d(%%rbp)\n", breglist[r], sym->st_posn);
      break;
    case P_INT:
      fprintf(Outfile, "\tmovl\t%s, %d(%%rbp)\n", dreglist[r], sym->st_posn);
      break;
    default:
      fatald("Bad type in cgstorlocal:", sym->type);
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
    fprintf(Outfile, "\t.globl\t%s\n", node->name);
  fprintf(Outfile, "%s:\n", node->name);

  // Output space for one or more elements
  for (i = 0; i < node->nelems; i++) {

    // Get any initial value
    initvalue = 0;
    if (node->initlist != NULL)
      initvalue = node->initlist[i];

    // Generate the space for this type
    switch (size) {
    case 1:
      fprintf(Outfile, "\t.byte\t%d\n", initvalue);
      break;
    case 4:
      fprintf(Outfile, "\t.long\t%d\n", initvalue);
      break;
    case 8:
      // Generate the pointer to a string literal. Treat a zero value
      // as actually zero, not the label L0
      if (node->initlist != NULL && type == pointer_to(P_CHAR)
	  && initvalue != 0)
	fprintf(Outfile, "\t.quad\tL%d\n", initvalue);
      else
	fprintf(Outfile, "\t.quad\t%d\n", initvalue);
      break;
    default:
      for (i = 0; i < size; i++)
	fprintf(Outfile, "\t.byte\t0\n");
    }
  }
}

// Generate a global string and its start label
// Don't output the label if append is true.
void cgglobstr(int l, char *strvalue, int append) {
  char *cptr;
  if (!append)
    cglabel(l);
  for (cptr = strvalue; *cptr; cptr++) {
    fprintf(Outfile, "\t.byte\t%d\n", *cptr);
  }
}

void cgglobstrend(void) {
  fprintf(Outfile, "\t.byte\t0\n");
}

// List of comparison instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *cmplist[] =
  { "sete", "setne", "setl", "setg", "setle", "setge" };

// Compare two registers and set if true.
int cgcompare_and_set(int ASTop, int r1, int r2, int type) {
  int size = cgprimsize(type);


  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in cgcompare_and_set()");

  switch (size) {
  case 1:
    fprintf(Outfile, "\tcmpb\t%s, %s\n", breglist[r2], breglist[r1]);
    break;
  case 4:
    fprintf(Outfile, "\tcmpl\t%s, %s\n", dreglist[r2], dreglist[r1]);
    break;
  default:
    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  }
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
int cgcompare_and_jump(int ASTop, int r1, int r2, int label, int type) {
  int size = cgprimsize(type);

  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in cgcompare_and_set()");

  switch (size) {
  case 1:
    fprintf(Outfile, "\tcmpb\t%s, %s\n", breglist[r2], breglist[r1]);
    break;
  case 4:
    fprintf(Outfile, "\tcmpl\t%s, %s\n", dreglist[r2], dreglist[r1]);
    break;
  default:
    fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  }
  fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
  freeall_registers(NOREG);
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

  // Only return a value if we have a value to return
  if (reg != NOREG) {
    // Deal with pointers here as we can't put them in
    // the switch statement
    if (ptrtype(sym->type))
      fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[reg]);
    else {
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
    }
  }

  cgjump(sym->st_endlabel);
}


// Generate code to load the address of an
// identifier into a variable. Return a new register
int cgaddress(struct symtable *sym) {
  int r = alloc_register();

  if (sym->class == C_GLOBAL ||
      sym->class == C_EXTERN || sym->class == C_STATIC)
    fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", sym->name, reglist[r]);
  else
    fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", sym->st_posn, reglist[r]);
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
  case 4:
    fprintf(Outfile, "\tmovslq\t(%s), %s\n", reglist[r], reglist[r]);
    break;
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
  case 4:
    fprintf(Outfile, "\tmovl\t%s, (%s)\n", dreglist[r1], reglist[r2]);
    break;
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
  fprintf(Outfile, "\tjmp\t__switch\n");
}

// Move value between registers
void cgmove(int r1, int r2) {
  fprintf(Outfile, "\tmovq\t%s, %s\n", reglist[r1], reglist[r2]);
}

void cglinenum(int line) {
  fprintf(Outfile, "\t.loc 1 %d 0\n", line);
}
