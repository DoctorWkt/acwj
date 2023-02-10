#include "defs.h"
#include "data.h"
#include "decl.h"

// Code generator for x86-64
// Copyright (c) 2019 Warren Toomey, GPL3

// Flag to say which section were are outputting in to
enum { no_seg, text_seg, data_seg } currSeg = no_seg;

// List of available registers and their names.
// We need a list of byte and doubleword registers, too.
// The list also includes the registers used to
// hold function parameters
#define NUMFREEREGS 4
#define FIRSTPARAMREG 9		// Position of first parameter register
static int freereg[NUMFREEREGS];
static char *reglist[] =
 { "r10",  "r11", "r12", "r13", "r9", "r8", "rcx", "rdx", "rsi",
   "rdi"
};

// We also need the 8-bit and 32-bit register names
static char *breglist[] =
 { "r10b",  "r11b", "r12b", "r13b", "r9b", "r8b", "cl", "dl", "sil",
   "dil"
};

static char *dreglist[] =
 { "r10d",  "r11d", "r12d", "r13d", "r9d", "r8d", "ecx", "edx",
  "esi", "edi"
};

// Switch to the text segment
void cgtextseg() {
  if (currSeg != text_seg) {
    fputs("\tsection .text\n", Outfile);
    currSeg = text_seg;
  }
}

// Switch to the data segment
void cgdataseg() {
  if (currSeg != data_seg) {
    fputs("\tsection .data\n", Outfile);
    currSeg = data_seg;
  }
}

// Generate a label
void cglabel(int l) {
  fprintf(Outfile, "L%d:\n", l);
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

// Store a register's value into a local variable
int cgstorlocal(int r, struct symtable *sym) {
  if (cgprimsize(sym->type) == 8) {
    fprintf(Outfile, "\tmov\tqword\t[rbp+%d], %s\n", sym->st_posn,
            reglist[r]);
  } else
  switch (sym->type) {
    case P_CHAR:
      fprintf(Outfile, "\tmov\tbyte\t[rbp+%d], %s\n", sym->st_posn,
              breglist[r]);
      break;
    case P_INT:
      fprintf(Outfile, "\tmov\tdword\t[rbp+%d], %s\n", sym->st_posn,
              dreglist[r]);
      break;
    default:
      fatald("Bad type in cgstorlocal:", sym->type);
  }
  return (r);
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
      // Align whatever we have now on a 4-byte alignment
      // I put the generic code here so it can be reused elsewhere
      alignment = 4;
      offset = (offset + direction * (alignment - 1)) & ~(alignment - 1);
  }
  return (offset);
}

// Position of next local variable relative to stack base pointer.
// We store the offset as positive to make aligning the stack pointer easier
static int localOffset;
// Position of stack pointer offset relative to stack base pointer.
// We need this to ensure it is aligned on a 16-byte boundary.
static int stackOffset;

// Create the position of a new local variable.
static int newlocaloffset(int size) {
  // Decrement the offset by a minimum of 4 bytes
  // and allocate on the stack
  localOffset += (size > 4) ? size : 4;
  return (-localOffset);
}

// Push and pop a register on/off the stack
static void pushreg(int r) {
  fprintf(Outfile, "\tpush\t%s\n", reglist[r]);
}

static void popreg(int r) {
  fprintf(Outfile, "\tpop\t%s\n", reglist[r]);
}

// Set all registers as available.
// But if reg is positive, don't free that one.

void cgfreeallregs(int keepreg) {
  int i;
  // fprintf(Outfile, "; freeing all registers\n");
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
int cgallocreg(void) {
  int reg;

  for (reg = 0; reg < NUMFREEREGS; reg++) {
    if (freereg[reg]) {
      freereg[reg] = 0;
      // fprintf(Outfile, "; allocated register %s\n", reglist[reg]);
      return (reg);
    }
  }
  // We have no registers, so we must spill one
  reg = (spillreg % NUMFREEREGS);
  spillreg++;
  // fprintf(Outfile, "; spilling reg %s\n", reglist[reg]);
  pushreg(reg);
  return (reg);
}

// Return a register to the list of available registers.
// Check to see if it's not already there.
void cgfreereg(int reg) {
  if (freereg[reg] != 0) {
    //fprintf(Outfile, "# error trying to free register %s\n", reglist[reg]);
    fatald("Error trying to free register", reg);
  }
  // If this was a spilled register, get it back
  if (spillreg > 0) {
    spillreg--;
    reg = (spillreg % NUMFREEREGS);
    // fprintf(Outfile, "; unspilling reg %s\n", reglist[reg]);
    popreg(reg);
  } else {
    // fprintf(Outfile, "; freeing reg %s\n", reglist[reg]);
    freereg[reg] = 1;
  }
}

// Spill all registers on the stack
void cgspillregs(void) {
  int i;

  for (i = 0; i < NUMFREEREGS; i++)
    pushreg(i);
}

// Unspill all registers from the stack
static void cgunspillregs(void) {
  int i;

  for (i = NUMFREEREGS - 1; i >= 0; i--)
    popreg(i);
}

// Print out the assembly preamble
// for one output file
void cgpreamble(char *filename) {
  cgfreeallregs(NOREG);
  cgtextseg();
  fprintf(Outfile, ";\t%s\n", filename);
  fprintf(Outfile,
	  "; internal switch(expr) routine\n"
	  "; rsi = switch table, rax = expr\n"
	  "; from SubC: http://www.t3x.org/subc/\n"
	  "\n"
	  "__switch:\n"
	  "        push   rsi\n"
	  "        mov    rsi, rdx\n"
	  "        mov    rbx, rax\n"
	  "        cld\n"
	  "        lodsq\n"
	  "        mov    rcx, rax\n"
	  "__next:\n"
	  "        lodsq\n"
	  "        mov    rdx, rax\n"
	  "        lodsq\n"
	  "        cmp    rbx, rdx\n"
	  "        jnz    __no\n"
	  "        pop    rsi\n"
	  "        jmp    rax\n"
	  "__no:\n"
	  "        loop   __next\n"
	  "        lodsq\n"
	  "        pop    rsi\n" "        jmp     rax\n\n");
}

// Nothing to do for the end of a file
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

  // Output the function start, save the rsp and rbp
//  if (sym->class == C_GLOBAL)
  if(!sym->extinit) {
    fprintf(Outfile, "\tglobal\t%s\n", name);
    sym->extinit = 1;
  }
  fprintf(Outfile,
	  "%s:\n" "\tpush\trbp\n"
	  "\tmov\trbp, rsp\n", name);

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
  fprintf(Outfile, "\tadd\trsp, %d\n", -stackOffset);
}

// Print out a function postamble
void cgfuncpostamble(struct symtable *sym) {
  cglabel(sym->st_endlabel);
  fprintf(Outfile, "\tadd\trsp, %d\n", stackOffset);
  fputs("\tpop	rbp\n" "\tret\n", Outfile);
  cgfreeallregs(NOREG);
}

// Load an integer literal value into a register.
// Return the number of the register.
// For x86-64, we don't need to worry about the type.
int cgloadint(int value, int type) {
  // Get a new register
  int r = cgallocreg();

  fprintf(Outfile, "\tmov\t%s, %d\n", reglist[r], value);
  return (r);
}

// Load a value from a variable into a register.
// Return the number of the register. If the
// operation is pre- or post-increment/decrement,
// also perform this action.
int cgloadvar(struct symtable *sym, int op) {
  int r, postreg, offset = 1;

  if(!sym->extinit) {
    fprintf(Outfile, "extern\t%s\n", sym->name);
    sym->extinit = 1;
  }

  // Get a new register
  r = cgallocreg();

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
      fprintf(Outfile, "\tlea\t%s, [rbp+%d]\n", reglist[r], sym->st_posn);
    else
      fprintf(Outfile, "\tlea\t%s, [%s]\n", reglist[r], sym->name);

    // and change the value at that address
    switch (sym->size) {
      case 1:
        fprintf(Outfile, "\tadd\tbyte [%s], %d\n", reglist[r], offset);
        break;
      case 4:
        fprintf(Outfile, "\tadd\tdword [%s], %d\n", reglist[r], offset);
        break;
      case 8:
        fprintf(Outfile, "\tadd\tqword [%s], %d\n", reglist[r], offset);
        break;
    }
  }

  // Now load the output register with the value
  if (sym->class == C_LOCAL || sym->class == C_PARAM) {
    switch (sym->size) {
      case 1:
        fprintf(Outfile, "\tmovzx\t%s, byte [rbp+%d]\n", reglist[r], sym->st_posn);
        break;
      case 4:
        fprintf(Outfile, "\tmovsxd\t%s, dword [rbp+%d]\n", reglist[r], sym->st_posn);
        break;
      case 8:
        fprintf(Outfile, "\tmov\t%s, [rbp+%d]\n", reglist[r], sym->st_posn);
    }
  } else {
    switch (sym->size) {
      case 1:
        fprintf(Outfile, "\tmovzx\t%s, byte [%s]\n", reglist[r], sym->name);
        break;
      case 4:
        fprintf(Outfile, "\tmovsxd\t%s, dword [%s]\n", reglist[r], sym->name);
        break;
      case 8:
        fprintf(Outfile, "\tmov\t%s, [%s]\n", reglist[r], sym->name);
    }
  }

  // If we have a post-operation, get a new register
  if (op == A_POSTINC || op == A_POSTDEC) {
    postreg = cgallocreg();

    // Load the symbol's address
    if (sym->class == C_LOCAL || sym->class == C_PARAM)
      fprintf(Outfile, "\tlea\t%s, [rbp+%d]\n", reglist[postreg], sym->st_posn);
    else
      fprintf(Outfile, "\tlea\t%s, [%s]\n", reglist[postreg], sym->name);

    // and change the value at that address
    switch (sym->size) {
      case 1:
        fprintf(Outfile, "\tadd\tbyte [%s], %d\n", reglist[postreg], offset);
        break;
      case 4:
        fprintf(Outfile, "\tadd\tdword [%s], %d\n", reglist[postreg], offset);
        break;
      case 8:
        fprintf(Outfile, "\tadd\tqword [%s], %d\n", reglist[postreg], offset);
        break;
    }

    // Finally, free the register
    cgfreereg(postreg);
  }

  // Return the register with the value
  return (r);
}

// Given the label number of a global string,
// load its address into a new register
int cgloadglobstr(int label) {
  // Get a new register
  int r = cgallocreg();
  fprintf(Outfile, "\tmov\t%s, L%d\n", reglist[r], label);
  return (r);
}

// Add two registers together and return
// the number of the register with the result
int cgadd(int r1, int r2) {
  fprintf(Outfile, "\tadd\t%s, %s\n", reglist[r1], reglist[r2]);
  cgfreereg(r2);
  return (r1);
}

// Subtract the second register from the first and
// return the number of the register with the result
int cgsub(int r1, int r2) {
  fprintf(Outfile, "\tsub\t%s, %s\n", reglist[r1], reglist[r2]);
  cgfreereg(r2);
  return (r1);
}

// Multiply two registers together and return
// the number of the register with the result
int cgmul(int r1, int r2) {
  fprintf(Outfile, "\timul\t%s, %s\n", reglist[r1], reglist[r2]);
  cgfreereg(r2);
  return (r1);
}

// Divide or modulo the first register by the second and
// return the number of the register with the result
int cgdivmod(int r1, int r2, int op) {
  fprintf(Outfile, "\tmov\trax, %s\n", reglist[r1]);
  fprintf(Outfile, "\tcqo\n");
  fprintf(Outfile, "\tidiv\t%s\n", reglist[r2]);
  if (op == A_DIVIDE)
    fprintf(Outfile, "\tmov\t%s, rax\n", reglist[r1]);
  else
    fprintf(Outfile, "\tmov\t%s, rdx\n", reglist[r1]);
  cgfreereg(r2);
  return (r1);
}

// Bitwise AND two registers
int cgand(int r1, int r2) {
  fprintf(Outfile, "\tand\t%s, %s\n", reglist[r1], reglist[r2]);
  cgfreereg(r2);
  return (r1);
}

// Bitwise OR two registers
int cgor(int r1, int r2) {
  fprintf(Outfile, "\tor\t%s, %s\n", reglist[r1], reglist[r2]);
  cgfreereg(r2);
  return (r1);
}

// Bitwise XOR two registers
int cgxor(int r1, int r2) {
  fprintf(Outfile, "\txor\t%s, %s\n", reglist[r1], reglist[r2]);
  cgfreereg(r2);
  return (r1);
}

// Shift left r1 by r2 bits
int cgshl(int r1, int r2) {
  fprintf(Outfile, "\tmov\tcl, %s\n", breglist[r2]);
  fprintf(Outfile, "\tshl\t%s, cl\n", reglist[r1]);
  cgfreereg(r2);
  return (r1);
}

// Shift right r1 by r2 bits
int cgshr(int r1, int r2) {
  fprintf(Outfile, "\tmov\tcl, %s\n", breglist[r2]);
  fprintf(Outfile, "\tshr\t%s, cl\n", reglist[r1]);
  cgfreereg(r2);
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

// Load a boolean value (only 0 or 1)
// into the given register
void cgloadboolean(int r, int val) {
  fprintf(Outfile, "\tmov\t%s, %d\n", reglist[r], val);
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
      fprintf(Outfile, "\tmovzx\t%s, byte %s\n", reglist[r], breglist[r]);
  }
  return (r);
}

// Call a function with the given symbol id.
// Pop off any arguments pushed on the stack.
// Return the register with the result
int cgcall(struct symtable *sym, int numargs) {
  int outr;

  // Call the function
  if(!sym->extinit) {
    fprintf(Outfile, "extern\t%s\n", sym->name);
    sym->extinit = 1;
  }
  fprintf(Outfile, "\tcall\t%s\n", sym->name);

  // Remove any arguments pushed on the stack
  if (numargs > 6) 
    fprintf(Outfile, "\tadd\trsp, %d\n", 8 * (numargs - 6));

  // Unspill all the registers
  cgunspillregs();

  // Get a new register and copy the return value into it
  outr = cgallocreg();
  fprintf(Outfile, "\tmov\t%s, rax\n", reglist[outr]);
  return (outr);
}

// Given a register with an argument value,
// copy this argument into the argposn'th
// parameter in preparation for a future function call.
// Note that argposn is 1, 2, 3, 4, ..., never zero.
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
  cgfreereg(r);
}

// Shift a register left by a constant
int cgshlconst(int r, int val) {
  fprintf(Outfile, "\tsal\t%s, %d\n", reglist[r], val);
  return (r);
}

// Store a register's value into a variable
int cgstorglob(int r, struct symtable *sym) {
  if(!sym->extinit) {
    fprintf(Outfile, "extern\t%s\n", sym->name);
    sym->extinit = 1;
  }
  if (cgprimsize(sym->type) == 8) {
    fprintf(Outfile, "\tmov\t[%s], %s\n", sym->name, reglist[r]);
  } else
  switch (sym->type) {
    case P_CHAR:
      fprintf(Outfile, "\tmov\t[%s], %s\n", sym->name, breglist[r]);
      break;
    case P_INT:
      fprintf(Outfile, "\tmov\t[%s], %s\n", sym->name, dreglist[r]);
      break;
    default:
      fatald("Bad type in cgloadglob:", sym->type);
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
    fprintf(Outfile, "\tglobal\t%s\n", node->name);
  if(!node->extinit) {
    node->extinit = 1;
  }
  fprintf(Outfile, "%s:\n", node->name);

  // Output space for one or more elements
  for (i = 0; i < node->nelems; i++) {

    // Get any initial value
    initvalue = 0;
    if (node->initlist != NULL)
      initvalue = node->initlist[i];

    // Generate the space for this type
    // original version
    switch(size) {
      case 1:
        fprintf(Outfile, "\tdb\t%d\n", initvalue);
        break;
      case 4:
        fprintf(Outfile, "\tdd\t%d\n", initvalue);
        break;
      case 8:
        // Generate the pointer to a string literal.  Treat a zero value
        // as actually zero, not the label L0
        if (node->initlist != NULL && type == pointer_to(P_CHAR) && initvalue != 0)
          fprintf(Outfile, "\tdq\tL%d\n", initvalue);
        else
          fprintf(Outfile, "\tdq\t%d\n", initvalue);
        break;
      default:
        for (i = 0; i < size; i++) 
          fprintf(Outfile, "\tdb\t0\n");
    }
  }

}

// Generate a global string and its start label.
// Don't output the label if append is true.
void cgglobstr(int l, char *strvalue, int append) {
  char *cptr;
  if (!append)
    cglabel(l);
  for (cptr = strvalue; *cptr; cptr++) {
    fprintf(Outfile, "\tdb\t%d\n", *cptr);
  }
}

// NULL terminate a global string
void cgglobstrend(void) {
  fprintf(Outfile, "\tdb\t0\n");
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
      fprintf(Outfile, "\tcmp\t%s, %s\n", breglist[r1], breglist[r2]);
      break;
    case 4:
      fprintf(Outfile, "\tcmp\t%s, %s\n", dreglist[r1], dreglist[r2]);
      break;
    default:
      fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
  }

  fprintf(Outfile, "\t%s\t%s\n", cmplist[ASTop - A_EQ], breglist[r2]);
  fprintf(Outfile, "\tmovzx\t%s, %s\n", reglist[r2], breglist[r2]);
  cgfreereg(r1);
  return (r2);
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
      fprintf(Outfile, "\tcmp\t%s, %s\n", breglist[r1], breglist[r2]);
      break;
    case 4:
      fprintf(Outfile, "\tcmp\t%s, %s\n", dreglist[r1], dreglist[r2]);
      break;
    default:
      fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
  }

  fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
  cgfreereg(r1);
  cgfreereg(r2);
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
      fprintf(Outfile, "\tmov\trax, %s\n", reglist[reg]);
    else {
      // Generate code depending on the function's type
      switch (sym->type) {
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
          fatald("Bad function type in cgreturn:", sym->type);
      }
    }
  }
  cgjump(sym->st_endlabel);
}

// Generate code to load the address of an
// identifier into a variable. Return a new register
int cgaddress(struct symtable *sym) {
  int r = cgallocreg();

  if (!sym->extinit) {
    fprintf(Outfile, "extern\t%s\n", sym->name);
    sym->extinit = 1;
  }
  if (sym->class == C_GLOBAL ||
      sym->class == C_EXTERN || sym->class == C_STATIC)
    fprintf(Outfile, "\tmov\t%s, %s\n", reglist[r], sym->name);
  else
    fprintf(Outfile, "\tlea\t%s, [rbp+%d]\n", reglist[r],
            sym->st_posn);
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
      fprintf(Outfile, "\tmovzx\t%s, byte [%s]\n", reglist[r], reglist[r]);
      break;
    case 4:
      fprintf(Outfile, "\tmovsx\t%s, dword [%s]\n", reglist[r], reglist[r]);
      break;
    case 8:
      fprintf(Outfile, "\tmov\t%s, [%s]\n", reglist[r], reglist[r]);
      break;
    default:
      fatald("Can't cgderef on type:", type);
  }
  return (r);
}

// Store through a dereferenced pointer
int cgstorderef(int r1, int r2, int type) {
  //Get the size of the type
  int size = cgprimsize(type);

  switch (size) {
    case 1:
      fprintf(Outfile, "\tmov\t[%s], byte %s\n", reglist[r2], breglist[r1]);
      break;
    case 4:
      fprintf(Outfile, "\tmov\t[%s], dword %s\n", reglist[r2], dreglist[r1]);
      break;
    case 8:
      fprintf(Outfile, "\tmov\t[%s], %s\n", reglist[r2], reglist[r1]);
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
  fprintf(Outfile, "\tdq\t%d\n", casecount);
  for (i = 0; i < casecount; i++)
    fprintf(Outfile, "\tdq\t%d, L%d\n", caseval[i], caselabel[i]);
  fprintf(Outfile, "\tdq\tL%d\n", defaultlabel);

  // Load the specific registers
  cglabel(toplabel);
  fprintf(Outfile, "\tmov\trax, %s\n", reglist[reg]);
  fprintf(Outfile, "\tmov\trdx, L%d\n", label);
  fprintf(Outfile, "\tjmp\t__switch\n");
}

// Move value between registers
void cgmove(int r1, int r2) {
  fprintf(Outfile, "\tmov\t%s, %s\n", reglist[r2], reglist[r1]);
}

// Output a gdb directive to say on which
// source code line number the following
// assembly code came from (not with nasm)
void cglinenum(int line) {
  //fprintf(Outfile, ";\t.loc 1 %d 0\n", line);
}
