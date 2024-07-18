#include "defs.h"
#include "data.h"
#include "gen.h"
#include "misc.h"
#include "types.h"
#include "target.h"

// Code generator for 6809
// Copyright (c) 2024 Warren Toomey, GPL3

// Instead of registers, we keep a list of locations.
// They can be one of the following:

enum {
  L_FREE,		// This location is not used
  L_SYMBOL,		// A global symbol with an optional offset
  L_LOCAL,		// A local variable or parameter
  L_CONST,		// An integer literal value
  L_LABEL,		// A label
  L_SYMADDR,		// The address of a symbol, local or parameter
  L_TEMP,		// A temporarily-stored value
  L_DREG		// The D location, i.e. B, D or Y/D
};

struct Location {
  int type;		// One of the L_ values
  char *name;		// A symbol's name
  long intval;		// Offset, const value, label-id etc.
  int primtype;		// 6809 primiive type, see PR_POINTER below
};

// We also track if D holds a copy of a location.
// It could be NOREG if it is available.
static int d_holds;

#define NUMFREELOCNS 16
static struct Location Locn[NUMFREELOCNS];

// We also need a set of temporary locations in memory.
// They are defined in crt0.s as R0, R1 etc.
// They can be allocated incrementally.
static int next_free_temp;

// Allocate a currently free temporary location
static int cgalloctemp() {
  return(next_free_temp++);
}

// Free all temporary locations
static void cgfreealltemps() {
  next_free_temp=0;
}

// Parameters and local variables live
// on the stack. We need to adjust their offset
// each time we push/pop off the stack. sp_adjust
// holds the number of extra bytes on the stack.
static int sp_adjust;

// We convert C types to types on the 6809:
// PR_CHAR, PR_INT, PR_LONG, PR_POINTER.
#define PR_CHAR		1
#define PR_INT		2
#define PR_POINTER	3
#define PR_LONG		4

// Given a C type, return the matching 6809 type
static int cgprimtype(int type) {
  if (ptrtype(type))  return(PR_POINTER);
  if (type == P_CHAR) return(PR_CHAR);
  if (type == P_INT)  return(PR_INT);
  if (type == P_LONG) return(PR_LONG);
  fatald("Bad type in cgprimtype:", type);
  return(0);		// Keep -Wall happy
}

// Print a location out. For memory locations
// use the offset. For constants, use the
// register letter to determine which part to use.
static void printlocation(int l, int offset, char rletter) {
  int intval;

  if (Locn[l].type == L_FREE)
    fatald("Error trying to print location", l);

  switch(Locn[l].type) {
    case L_SYMBOL: fprintf(Outfile, "_%s+%d\n", Locn[l].name, offset); break;
    case L_LOCAL: fprintf(Outfile, "%ld,s\n",
		Locn[l].intval + offset + sp_adjust);
	break;
    case L_LABEL: fprintf(Outfile, "#L%ld\n", Locn[l].intval); break;
    case L_SYMADDR: fprintf(Outfile, "#_%s\n", Locn[l].name); break;
    case L_TEMP: fprintf(Outfile, "R%ld+%d\n", Locn[l].intval, offset);
	break;
    case L_CONST:
      // We convert Locn[l].intval (a long) to intval (an int). If
      // we did, for example, Locn[l].intval & 0xffff, on the 6809
      // the 0xffff gets widened to 32 bits. But this is a negative
      // value, so it gets widened to 0xffffffff not 0x0000ffff.
      switch(rletter) {
	case 'b':
	  fprintf(Outfile, "#%ld\n", Locn[l].intval & 0xff); break;
	case 'a':
	  fprintf(Outfile, "#%ld\n", (Locn[l].intval >> 8) & 0xff); break;
	case 'd':
	  intval= (int)Locn[l].intval;
	  fprintf(Outfile, "#%d\n", intval & 0xffff); break;
	case 'y':
	  intval= (int)(Locn[l].intval >> 16);
	  fprintf(Outfile, "#%d\n", intval & 0xffff); break;

	// These are the top two bytes of a 32-bit value
	case 'f':
	  fprintf(Outfile, "#%ld\n", (Locn[l].intval >> 16) & 0xff); break;
	case 'e':
	  fprintf(Outfile, "#%ld\n", (Locn[l].intval >> 24) & 0xff); break;
      }
      break;
    default: fatald("Unknown type for location", l);
  }
}

// Save D (B, D, Y/D) to a location.
static void save_d(int l) {

  // If we are saving to ourself then
  // there is nothing to do :-)
  if (Locn[l].type == L_DREG) return;

  switch (Locn[l].primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\tstb "); printlocation(l, 0, 'b');
      break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\tstd "); printlocation(l, 0, 'd');
      break;
    case PR_LONG:
      fprintf(Outfile, "\tstd "); printlocation(l, 2, 'd');
      fprintf(Outfile, "\tsty "); printlocation(l, 0, 'y');
  }
  d_holds= l;
}

// Stash D in a temporary if required
static void stash_d() {
  // If D holds a value, we will need to store it
  // in a temporary location
  if (d_holds != NOREG && Locn[d_holds].type == L_DREG) {
    Locn[d_holds].type= L_TEMP;
    Locn[d_holds].intval= cgalloctemp();
    save_d(d_holds);
  }
}

// Load D (B, D, Y/D) with a location.
static void load_d(int l) {
  // If l is already L_DREG, do nothing.
  if (Locn[l].type== L_DREG) return;

  // If D holds a value, we will need to store it
  // in a temporary location
  stash_d();

  // Load the existing location into D and mark it as L_DREG.
  switch(Locn[l].primtype) {
    case PR_CHAR:
	fprintf(Outfile, "\tldb "); printlocation(l, 0, 'b'); break;
    case PR_INT:
    case PR_POINTER:
	fprintf(Outfile, "\tldd "); printlocation(l, 0, 'd'); break;
    case PR_LONG:
	fprintf(Outfile, "\tldd "); printlocation(l, 2, 'd');
	fprintf(Outfile, "\tldy "); printlocation(l, 0, 'y');
  }

  Locn[l].type= L_DREG;
  d_holds= l;
}

// Set all locations as available.
// If keepl is positive, don't free that one.
static void cgfreeall_locns(int keepl) {
  int l;

  for (l = 0; l < NUMFREELOCNS; l++)
    if (l != keepl) {
      Locn[l].type= L_FREE;
    }

  if (keepl == NOREG)
    cgfreealltemps();
  fprintf(Outfile, ";\n");
  d_holds= NOREG;
}

// Allocate a free location. Return the number of
// the location. Die if no available locations.
static int cgalloclocn(int type, int primtype, char *name, long intval) {
  int l;

  for (l = 0; l < NUMFREELOCNS; l++) {
    if (Locn[l].type== L_FREE) {

      // If we're asked for a temporary, get one
      if (type==L_TEMP)
	intval= cgalloctemp();
      if (type==L_DREG)
	d_holds= l;
      Locn[l].type= type;
      Locn[l].primtype= primtype;
      Locn[l].name= name;
      Locn[l].intval= intval;
      return(l);
    }
  }

  fatal("Out of locations in cgalloclocn");
  return(0);	// Keep -Wall happy
}

// Free a location. Check to see if it's not already there.
static void cgfreelocn(int l) {
  if (Locn[l].type== L_FREE)
    fatald("Error trying to free location", l);
  Locn[l].type= L_FREE;
  if (d_holds ==l) d_holds= NOREG;
}

// gen.c calls us as if we have registers
void cgfreeallregs(int keepl) {
  cgfreeall_locns(keepl);
}

int cgallocreg(int type) {
  return(cgalloclocn(L_TEMP, cgprimtype(type), NULL, 0));
}

void cgfreereg(int reg) {
  cgfreelocn(reg);
}

// Push a location on the stack
static void pushlocn(int l) {
  load_d(l);

  switch(Locn[l].primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\tpshs b\n");
      sp_adjust += 1;
      break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\tpshs d\n");
      sp_adjust += 2;
      break;
    case PR_LONG:
      fprintf(Outfile, "\tpshs d\n");
      fprintf(Outfile, "\tpshs y\n");
      sp_adjust += 4;
  }

  cgfreelocn(l);
  d_holds= NOREG;
}

// Flag to say which section were are outputting in to
enum { no_seg, text_seg, data_seg, lit_seg } currSeg = no_seg;

// Switch to the text segment
void cgtextseg() {
  if (currSeg != text_seg) {
    fputs("\t.code\n", Outfile);
    currSeg = text_seg;
  }
}

// Switch to the data segment
void cgdataseg() {
  if (currSeg != data_seg) {
    fputs("\t.data\n", Outfile);
    currSeg = data_seg;
  }
}

// Switch to the literal segment
void cglitseg() {
  if (currSeg != lit_seg) {
    fputs("\t.literal\n", Outfile);
    currSeg = lit_seg;
  }
}

// Position of next local variable relative to stack base pointer.
// We store the offset as positive to make aligning the stack pointer easier
static int localOffset;

// Create the position of a new local variable.
static int newlocaloffset(int size) {
  int o;

  // Return the current localOffset and
  // then increment the localOffset
  o= localOffset;
  localOffset += size;
  return (o);
}


// Print out the assembly preamble
// for one output file
void cgpreamble() {
  cgfreeall_locns(NOREG);
  cgfreealltemps();
  cgtextseg();
}

// Nothing to do for the end of a file
void cgpostamble() {
}

// Generate a label
void cglabel(int l) {
  fprintf(Outfile, "L%d:\n", l);
}

// Print out a function preamble
void cgfuncpreamble(struct symtable *sym) {
  char *name = sym->name;
  struct symtable *parm, *locvar;
  int paramOffset = 2;	// Any pushed params start at this frame offset

  // Output in the text segment, reset local offset
  // and the amount of args on the stack
  cgtextseg();
  localOffset = 0;
  next_free_temp = 0;
  sp_adjust = 0;

  // Output the function start
  if (sym->class == V_GLOBAL) {
    fprintf(Outfile, "\t.export _%s\n", name);
  }
  fprintf(Outfile, "_%s:\n", name);

  // Make frame positions for the locals.
  // Skip over the parameters in the member list first
  for (locvar = sym->member; locvar != NULL; locvar = locvar->next)
    if (locvar->class==V_LOCAL) break;

  for (; locvar != NULL; locvar = locvar->next) {
    locvar->st_posn = newlocaloffset(locvar->size);
    // fprintf(Oufile, "; placed local %s size %d at offset %d\n",
    //			locvar->name, locvar->size, locvar->st_posn);
  }

  // Work out the frame offset for the parameters.
  // Do this once we know the total size of the locals.
  // Stop once we hit the locals
  for (parm = sym->member; parm != NULL; parm = parm->next) {
    if (parm->class==V_LOCAL) break;
    parm->st_posn = paramOffset + localOffset;
    paramOffset += parm->size;
    // fprintf(Outfile, "; placed param %s size %d at offset %d\n",
    // 				parm->name, parm->size, parm->st_posn);
  }

  // Bring the stack down to below the locals
  if (localOffset!=0)
    fprintf(Outfile, "\tleas -%d,s\n", localOffset);
}

// Print out a function postamble
void cgfuncpostamble(struct symtable *sym) {
  cglabel(sym->st_endlabel);
  if (localOffset!=0)
    fprintf(Outfile, "\tleas %d,s\n", localOffset);
  fputs("\trts\n", Outfile);
  cgfreeall_locns(NOREG);
  cgfreealltemps();

  if (sp_adjust !=0 ) {
    fprintf(Outfile, "; DANGER sp_adjust is %d not 0\n", sp_adjust);
    fatald("sp_adjust is not zero", sp_adjust);
  }
}

// Load an integer literal value into a location.
// Return the number of the location.
int cgloadint(int value, int type) {
  int primtype= cgprimtype(type);
  return(cgalloclocn(L_CONST, primtype, NULL, value));
}

// Increment the value at a symbol by offset
// which could be positive or negative
static void incdecsym(struct symtable *sym, int offset) {
    // Load the symbol's address
    if (sym->class == V_LOCAL || sym->class == V_PARAM)
      fprintf(Outfile, "\tleax %d,s\n", sym->st_posn + sp_adjust);
    else
      fprintf(Outfile, "\tldx #_%s\n", sym->name);

    // Now change the value at that address
    switch (sym->size) {
    case 1:
      fprintf(Outfile, "\tldb #%d\n", offset & 0xff);
      fprintf(Outfile, "\taddb 0,x\n");
      fprintf(Outfile, "\tstb 0,x\n");
      break;
    case 2:
      fprintf(Outfile, "\tldd #%d\n", offset & 0xffff);
      fprintf(Outfile, "\taddd 0,x\n");
      fprintf(Outfile, "\tstd 0,x\n");
      break;
    case 4:
      fprintf(Outfile, "\tldd #%d\n", offset);
      fprintf(Outfile, "\taddd 2,x\n");
      fprintf(Outfile, "\tstd 2,x\n");
      fprintf(Outfile, "\tldd 0,x\n");
      fprintf(Outfile, "\tadcb #0\n");
      fprintf(Outfile, "\tadca #0\n");
      fprintf(Outfile, "\tstb 0,x\n");
    }
}

// Load a value from a variable into a location.
// Return the number of the location. If the
// operation is pre- or post-increment/decrement,
// also perform this action.
int cgloadvar(struct symtable *sym, int op) {
  int l, offset = 1;
  int primtype= cgprimtype(sym->type);

  // If the symbol is a pointer, use the size
  // of the type that it points to as any
  // increment or decrement. If not, it's one.
  if (ptrtype(sym->type))
    offset = typesize(value_at(sym->type), sym->ctype);

  // Negate the offset for decrements
  if (op == A_PREDEC || op == A_POSTDEC)
    offset = -offset;

  // If we have a pre-operation, do it
  if (op == A_PREINC || op == A_PREDEC)
    incdecsym(sym, offset);

  // Get a new location and set it up
  if (sym->class == V_LOCAL || sym->class == V_PARAM)
    l= cgalloclocn(L_LOCAL, primtype, NULL, sym->st_posn + sp_adjust);
  else
    l= cgalloclocn(L_SYMBOL, primtype, sym->name, 0);

  // If we have a post-operation, do it
  // but get the current value into a temporary.
  if (op == A_POSTINC || op == A_POSTDEC) {
    load_d(l);
    stash_d();
    incdecsym(sym, offset);
    load_d(l);
  }

  // Return the location with the value
  return (l);
}

// Given the label number of a global string,
// load its address into a new location
int cgloadglobstr(int label) {
  // Get a new location
  int l = cgalloclocn(L_LABEL, PR_INT, NULL, label);
  return (l);
}

// Add two locations together and return
// the number of the location with the result
int cgadd(int l1, int l2, int type) {
  int primtype= cgprimtype(type);

  load_d(l1);

  switch(primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\taddb "); printlocation(l2, 0, 'b'); break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\taddd "); printlocation(l2, 0, 'd'); break;
      break;
    case PR_LONG:
      fprintf(Outfile, "\taddd "); printlocation(l2, 2, 'd');
      fprintf(Outfile, "\texg y,d\n");
      fprintf(Outfile, "\tadcb "); printlocation(l2, 1, 'f');
      fprintf(Outfile, "\tadca "); printlocation(l2, 0, 'e');
      fprintf(Outfile, "\texg y,d\n");
  }
  cgfreelocn(l2);
  Locn[l1].type= L_DREG;
  d_holds= l1;
  return(l1);
}

// Subtract the second location from the first and
// return the number of the location with the result
int cgsub(int l1, int l2, int type) {
  int primtype= cgprimtype(type);

  load_d(l1);

  switch(primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\tsubb "); printlocation(l2, 0, 'b'); break;
      break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\tsubd "); printlocation(l2, 0, 'd'); break;
      break;
    case PR_LONG:
      fprintf(Outfile, "\tsubd "); printlocation(l2, 2, 'd');
      fprintf(Outfile, "\texg y,d\n");
      fprintf(Outfile, "\tsbcb "); printlocation(l2, 1, 'f');
      fprintf(Outfile, "\tsbca "); printlocation(l2, 0, 'e');
      fprintf(Outfile, "\texg y,d\n");
  }
  cgfreelocn(l2);
  Locn[l1].type= L_DREG;
  d_holds= l1;
  return (l1);
}

// Run a helper subroutine on two locations
// and return the number of the location with the result
static int cgbinhelper(int l1, int l2, int type,
				char *cop, char *iop, char *lop) {
  int primtype= cgprimtype(type);

  load_d(l1);

  switch(primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\tclra\n");
      fprintf(Outfile, "\tpshs d\n");
      sp_adjust += 2;
      fprintf(Outfile, "\tldb "); printlocation(l2, 0, 'b');
      fprintf(Outfile, "\tlbsr %s\n", cop);
      sp_adjust -= 2;
      break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\tpshs d\n");
      sp_adjust += 2;
      fprintf(Outfile, "\tldd "); printlocation(l2, 0, 'd');
      fprintf(Outfile, "\tlbsr %s\n", iop);
      sp_adjust -= 2;
      break;
    case PR_LONG:
      fprintf(Outfile, "\tpshs d\n");
      fprintf(Outfile, "\tpshs y\n");
      sp_adjust += 4;
      fprintf(Outfile, "\tldy "); printlocation(l2, 0, 'd');
      fprintf(Outfile, "\tldd "); printlocation(l2, 2, 'y');
      fprintf(Outfile, "\tlbsr %s\n", lop);
      sp_adjust -= 4;
  }
  cgfreelocn(l2);
  Locn[l1].type= L_DREG;
  d_holds= l1;
  return (l1);
}

// Multiply two locations together and return
// the number of the location with the result
int cgmul(int r1, int r2, int type) {
  return(cgbinhelper(r1, r2, type, "__mul", "__mul", "__mull"));
}

// Divide the first location by the second and
// return the number of the location with the result
int cgdiv(int r1, int r2, int type) {
  return(cgbinhelper(r1, r2, type, "__div", "__div", "__divl"));
}

// Divide the first location by the second to get
// the remainder. Return the number of the location with the result
int cgmod(int r1, int r2, int type) {
  return(cgbinhelper(r1, r2, type, "__rem", "__rem", "__reml"));
}

// Generic binary operation on two locations
static int cgbinop(int l1, int l2, int type, char *op) {
  int primtype= cgprimtype(type);

  load_d(l1);

  switch(primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\t%sb ", op); printlocation(l2, 0, 'b');
      break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\t%sa ", op); printlocation(l2, 0, 'a');
      fprintf(Outfile, "\t%sb ", op); printlocation(l2, 1, 'b');
      break;
    case PR_LONG:
      fprintf(Outfile, "\t%sa ", op); printlocation(l2, 2, 'a');
      fprintf(Outfile, "\t%sb ", op); printlocation(l2, 3, 'b');
      fprintf(Outfile, "\texg y,d\n");
      fprintf(Outfile, "\t%sa ", op); printlocation(l2, 0, 'e');
      fprintf(Outfile, "\t%sb ", op); printlocation(l2, 1, 'f');
      fprintf(Outfile, "\texg y,d\n");
      break;
  }
  cgfreelocn(l2);
  Locn[l1].type= L_DREG;
  d_holds= l1;
  return (l1);
}

// Bitwise AND two locations
int cgand(int r1, int r2, int type) {
  return(cgbinop(r1, r2, type, "and"));
}

// Bitwise OR two locations
int cgor(int r1, int r2, int type) {
  return(cgbinop(r1, r2, type, "or"));
}

// Bitwise XOR two locations
int cgxor(int r1, int r2, int type) {
  return(cgbinop(r1, r2, type, "eor"));
}

// Invert a location's value
int cginvert(int l, int type) {
  int primtype= cgprimtype(type);

  load_d(l);

  switch(primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\tcomb\n");
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\tcoma\n");
      fprintf(Outfile, "\tcomb\n");
      break;
    case PR_LONG:
      fprintf(Outfile, "\tcoma\n");
      fprintf(Outfile, "\tcomb\n");
      fprintf(Outfile, "\texg y,d\n");
      fprintf(Outfile, "\tcoma\n");
      fprintf(Outfile, "\tcomb\n");
      fprintf(Outfile, "\texg y,d\n");
  }

  Locn[l].type= L_DREG;
  d_holds= l;
  return(l);
}

// Shift left r1 by r2 bits
int cgshl(int r1, int r2, int type) {
  return(cgbinhelper(r1, r2, type, "__shl", "__shl", "__shll"));
}

// Shift r1 right by 8, 16 or 24 bits
int cgshrconst(int r1, int amount, int type) {
  int primtype= cgprimtype(type);
  int temp;

  load_d(r1);

  switch(primtype) {
    // Any shift on B clears it
    case PR_CHAR:
      fprintf(Outfile, "\tclrb\n"); return(r1);
    case PR_INT:
    case PR_POINTER:
      switch(amount) {
	case  8:
          fprintf(Outfile, "\ttfr a,b\n");
          fprintf(Outfile, "\tclra\n"); return(r1);
	case 16:
	case 24:
          fprintf(Outfile, "\tclra\n");
          fprintf(Outfile, "\tclrb\n"); return(r1);
      }
    case PR_LONG:
      switch(amount) {
	case  8:
          temp= cgalloctemp();
	  fprintf(Outfile, "\tclr R%d ; long >> 8\n", temp);
	  fprintf(Outfile, "\tsty R%d+1\n", temp);
	  fprintf(Outfile, "\tsta R%d+3\n", temp);
	  fprintf(Outfile, "\tldy R%d\n", temp);
	  fprintf(Outfile, "\tldd R%d+2\n", temp); return(r1);
	case 16:
          fprintf(Outfile, "\ttfr y,d ; long >> 16\n");
	  fprintf(Outfile, "\tldy #0\n"); return(r1);
	case 24:
          fprintf(Outfile, "\ttfr y,d ; long >> 24\n");
          fprintf(Outfile, "\ttfr a,b\n");
          fprintf(Outfile, "\tclra\n");
	  fprintf(Outfile, "\tldy #0\n"); return(r1);
      }
  }
  return(0);	// Keep -Wall happy
}

// Shift right r1 by r2 bits
int cgshr(int r1, int r2, int type) {
  int val;

  // If r2 is the constant 8, 16 or 24
  // we can do it with just a few instructions
  if (Locn[r2].type== L_CONST) {
    val= (int)Locn[r2].intval;
    if (val==8 || val==16 || val==24)
      return(cgshrconst(r1, val, type));
  }

  return(cgbinhelper(r1, r2, type, "__shr", "__shr", "__shrl"));
}

// Negate a location's value
int cgnegate(int l, int type) {
  int primtype= cgprimtype(type);

  load_d(l);

  switch(primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\tnegb\n");
      break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\tcoma\n");
      fprintf(Outfile, "\tcomb\n");
      fprintf(Outfile, "\taddd #1\n");
      break;
    case PR_LONG:
      fprintf(Outfile, "\tlbsr __negatel\n");
  }
  Locn[l].type= L_DREG;
  d_holds= l;
  return (l);
}

// Logically negate a location's value
int cglognot(int l, int type) {
  // Get two labels
  int label1 = genlabel();
  int label2 = genlabel();
  int primtype= cgprimtype(type);

  load_d(l);

  switch(primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\tcmpb #0\n");
      fprintf(Outfile, "\tbne L%d\n", label1);
      fprintf(Outfile, "\tldd #1\n");
      break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\tcmpd #0\n");
      fprintf(Outfile, "\tbne L%d\n", label1);
      fprintf(Outfile, "\tldd #1\n");
      break;
    case PR_LONG:
      fprintf(Outfile, "\tcmpd #0\n");
      fprintf(Outfile, "\tbne L%d\n", label1);
      fprintf(Outfile, "\tcmpy #0\n");
      fprintf(Outfile, "\tbne L%d\n", label1);
      fprintf(Outfile, "\tldd #1\n");
  }
  fprintf(Outfile, "\tbra L%d\n", label2);
  cglabel(label1);
  fprintf(Outfile, "\tldd #0\n");
  cglabel(label2);

  Locn[l].type= L_DREG;
  d_holds= l;
  return (l);
}

// Load a boolean value (only 0 or 1)
// into the given location. Allocate
// a location if l is NOREG.
int cgloadboolean(int l, int val, int type) {
  int primtype= cgprimtype(type);
  int templ;

  // Put the value into a literal location.
  // Load it into D.
  templ= cgalloclocn(L_CONST, primtype, NULL, val);
  load_d(templ);

  // Return the literal location or
  // save the value and return that location
  if (l==NOREG) {
    return(templ);
  } else {
    save_d(l);
    return(l);
  }
  return(NOREG);	// Keep -Wall happy
}

// Set the Z flag if D is already loaded. Otherwise,
// load the D register which will set the Z flag
static void load_d_z(int l, int type) {
  int primtype= cgprimtype(type);
  int label = genlabel();

  if (Locn[l].type== L_DREG) {
    switch(primtype) {
      case PR_CHAR:
        fprintf(Outfile, "\tcmpb #0\n");
        break;
      case PR_INT:
      case PR_POINTER:
        fprintf(Outfile, "\tcmpd #0\n");
        break;
      case PR_LONG:
        fprintf(Outfile, "\tcmpd #0\n");
        fprintf(Outfile, "\tbne L%d\n", label);
        fprintf(Outfile, "\tcmpy #0\n");
        cglabel(label);
    }
  } else
    load_d(l);
}

// Convert an integer value to a boolean value for
// a TOBOOL operation. Jump if true if it's an IF,
// WHILE operation. Jump if false if it's
// a LOGOR operation.
int cgboolean(int l, int op, int label, int type) {
  int primtype= cgprimtype(type);
  char *jmpop= "beq";

  if (op== A_LOGOR) jmpop= "bne";

  load_d_z(l, type);

  switch(primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\t%s L%d\n", jmpop, label);
      break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\t%s L%d\n", jmpop, label);
      break;
    case PR_LONG:
      fprintf(Outfile, "\tpshs y\n");
      fprintf(Outfile, "\tora 0,s\n");
      fprintf(Outfile, "\torb 1,s\n");
      fprintf(Outfile, "\tleas 2,s\n");
      fprintf(Outfile, "\t%s L%d\n", jmpop, label);
  }

  // If the op is A_TOBOOL, set the location to 1
  if (op == A_TOBOOL) {
    cgloadboolean(l, 1, type);
    return(l);
  }
  return(NOREG);
}

// Call a function with the given symbol id.
// Beforehand, push the arguments on the stack.
// Afterwards, pop off any arguments pushed on the stack.
// Return the location with the result.
int cgcall(struct symtable *sym, int numargs, int *arglist, int *typelist) {
  int i, l, argamount;
  int gentype= sym->type;
  int primtype= 0;

  // If it's not a void function, get its primtype.
  // Also stash any D value in a temporary.
  if (gentype!=P_VOID) {
    stash_d();
    primtype= cgprimtype(sym->type);
  }

  // Push the function arguments on the stack
  argamount=0;
  for (i= 0; i< numargs; i++) {
    pushlocn(arglist[i]);
    argamount += cgprimsize(typelist[i]);
  }

  // Call the function, adjust the stack
  fprintf(Outfile, "\tlbsr _%s\n", sym->name);
  fprintf(Outfile, "\tleas %d,s\n", argamount);
  sp_adjust -= argamount;

  // If it's not a void function, mark the result in D
  if (gentype!=P_VOID) {
    // Get a location and say that D is in use
    l = cgalloclocn(L_DREG, primtype, NULL, 0);
    return (l);
  }
  return(NOREG);
}

// Shift a location left by a constant, only 1 or 2
int cgshlconst(int l, int val, int type) {
  int primtype= cgprimtype(type);

  load_d(l);

  switch(primtype) {
    case PR_CHAR:
      if (val==2) {
        fprintf(Outfile, "\taslb\n");
      }
      fprintf(Outfile, "\taslb\n");
      break;
    case PR_INT:
    case PR_POINTER:
      if (val==2) {
        fprintf(Outfile, "\taslb\n");
        fprintf(Outfile, "\trola\n");
      }
      fprintf(Outfile, "\taslb\n");
      fprintf(Outfile, "\trola\n");
      break;
    case PR_LONG:
      if (val==2) {
        fprintf(Outfile, "\taslb\n");
        fprintf(Outfile, "\trola\n");
      }
      fprintf(Outfile, "\texg y,d\n");
      fprintf(Outfile, "\trolb\n");
      fprintf(Outfile, "\trola\n");
      fprintf(Outfile, "\texg y,d\n");
  }
  Locn[l].type= L_DREG;
  d_holds= l;
  return (l);
}

// Store a location's value into a variable
int cgstorglob(int l, struct symtable *sym) {
  int size= cgprimsize(sym->type);

  load_d(l);

  switch (size) {
    case 1:
      fprintf(Outfile, "\tstb _%s\n", sym->name);
      break;
    case 2:
      fprintf(Outfile, "\tstd _%s\n", sym->name);
      break;
    case 4:
      fprintf(Outfile, "\tstd _%s+2\n", sym->name);
      fprintf(Outfile, "\tsty _%s\n", sym->name);
  }
  return (l);
}

// Store a location's value into a local variable
int cgstorlocal(int l, struct symtable *sym) {
  int primtype= cgprimtype(sym->type);

  load_d(l);

  switch (primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\tstb %d,s\n", sym->st_posn + sp_adjust);
      break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\tstd %d,s\n", sym->st_posn + sp_adjust);
      break;
    case PR_LONG:
      fprintf(Outfile, "\tsty %d,s\n", sym->st_posn + sp_adjust);
      fprintf(Outfile, "\tstd %d,s\n", 2+sym->st_posn + sp_adjust);
  }
  return (l);
}

// Generate a global symbol but not functions
void cgglobsym(struct symtable *node) {
  int size, type;
  int initvalue;
  int i,j;

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
    fprintf(Outfile, "\t.export _%s\n", node->name);
  fprintf(Outfile, "_%s:\n", node->name);

  // Output space for one or more elements
  for (i = 0; i < node->nelems; i++) {

    // Get any initial value
    initvalue = 0;
    if (node->initlist != NULL)
      initvalue = node->initlist[i];

    // Generate the space for this type
    switch (size) {
    case 1:
      fprintf(Outfile, "\t.byte\t%d\n", initvalue & 0xff);
      break;
    case 2:
      // Generate the pointer to a string literal. Treat a zero value
      // as actually zero, not the label L0
      if (node->initlist != NULL && type == pointer_to(P_CHAR)
	  && initvalue != 0)
	fprintf(Outfile, "\t.word\tL%d\n", initvalue);
      else
        fprintf(Outfile, "\t.word\t%d\n", initvalue & 0xffff);
      break;
    case 4:
	fprintf(Outfile, "\t.word\t%d\n", (initvalue >> 16) & 0xffff);
	fprintf(Outfile, "\t.word\t%d\n", initvalue & 0xffff);
      break;
    default:
      for (j = 0; j < size; j++)
	fprintf(Outfile, "\t.byte\t0\n");
    }
  }
}

// Generate a global string and its start label.
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
  { "beq", "bne", "blt", "bgt", "ble", "bge" };

// Compare two locations and set if true.
int cgcompare_and_set(int ASTop, int l1, int l2, int type) {
  int label1, label2;
  int primtype = cgprimtype(type);

  // Get two labels
  label1= genlabel();
  label2= genlabel();

  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in cgcompare_and_set()");

  load_d(l1);

  switch (primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\tcmpb "); printlocation(l2, 0, 'b');
      break;
    case PR_INT:
    case PR_POINTER:
    case PR_LONG:
      fprintf(Outfile, "\tcmpd "); printlocation(l2, 0, 'd');
      break;
  }

  fprintf(Outfile, "\t%s L%d\n", cmplist[ASTop - A_EQ], label1);

  // XXX This isn't right and I need to fix it
  if (primtype==PR_LONG) {
    fprintf(Outfile, "\tbne L%d\n", label1);
    fprintf(Outfile, "\tcmpd "); printlocation(l2, 2, 'd');
  }
  fprintf(Outfile, "\tldd #0\n");
  fprintf(Outfile, "\tbra L%d\n", label2);
  cglabel(label1);
  fprintf(Outfile, "\tldd #1\n");
  cglabel(label2);
  cgfreelocn(l2);

  // Mark the location as the D register
  Locn[l1].type= L_DREG;
  d_holds= l1;
  return (l1);
}

// Generate a jump to a label
void cgjump(int l) {
  fprintf(Outfile, "\tbra L%d\n", l);
  d_holds= NOREG;
}

// List of inverted jump instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *invcmplist[] = { "bne", "beq", "bge", "ble", "bgt", "blt" };

// Comparisons used by the long function
static char *lcmplist1[] = { "bne", "beq", "bge", "ble", "bgt", "blt" };
static char *lcmplist2[] = { "bne", "beq", "bhs", "bls", "bhi", "blo" };

// Compare two long locations and jump if false
static void longcmp_and_jump(int ASTop, int parentASTop,
					int l1, int l2, int label) {
  int truelabel;

  // Generate a new label
  truelabel=genlabel();

  fprintf(Outfile, "\t%s L%d\n", lcmplist1[ASTop - A_EQ], label);
  switch(ASTop) {
    case A_EQ:
      fprintf(Outfile, "\tbne L%d\n", label);
      break;
    case A_NE:
      fprintf(Outfile, "\tbne L%d\n", truelabel);
      break;
    case A_LT:
      fprintf(Outfile, "\tblt L%d\n", truelabel);
      fprintf(Outfile, "\tbne L%d\n", label);
      break;
    case A_GT:
      fprintf(Outfile, "\tbgt L%d\n", truelabel);
      fprintf(Outfile, "\tbne L%d\n", label);
      break;
    case A_LE:
      fprintf(Outfile, "\tbgt L%d\n", label);
      fprintf(Outfile, "\tbne L%d\n", truelabel);
      break;
    case A_GE:
      fprintf(Outfile, "\tblt L%d\n", label);
      fprintf(Outfile, "\tbne L%d\n", truelabel);
  }

  fprintf(Outfile, "\tcmpd "); printlocation(l2, 2, 'd');
  fprintf(Outfile, "\t%s L%d\n", lcmplist2[ASTop - A_EQ], label);
  cglabel(truelabel);
}

// Compare two locations and jump if false.
// Jump if true if the parent op is A_LOGOR
int cgcompare_and_jump(int ASTop, int parentASTop,
				int l1, int l2, int label, int type) {
  int primtype = cgprimtype(type);
  char *jmpop;

  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in cgcompare_and_set()");

  load_d_z(l1, type);
  jmpop= invcmplist[ASTop - A_EQ];
  if (parentASTop==A_LOGOR)
    jmpop= cmplist[ASTop - A_EQ];

  switch (primtype) {
    case PR_CHAR:
      fprintf(Outfile, "\tcmpb "); printlocation(l2, 0, 'b'); break;
    case PR_INT:
    case PR_POINTER:
      fprintf(Outfile, "\tcmpd "); printlocation(l2, 0, 'd'); break;
    case PR_LONG:
      fprintf(Outfile, "\tcmpy "); printlocation(l2, 0, 'y'); break;
  }

  if (primtype==PR_LONG) longcmp_and_jump(ASTop, parentASTop, l1, l2, label);
  fprintf(Outfile, "\t%s L%d\n", jmpop, label);
  cgfreelocn(l1);
  cgfreelocn(l2);
  return (NOREG);
}

// Widen the value in the location from the old
// to the new type, and return a location
// with this new value
int cgwiden(int l, int oldtype, int newtype) {
  int how= cgprimsize(newtype) - cgprimsize(oldtype);
  int label1, label2;
  int l2;

  // If the sizes are the same do nothing
  if (how==0) return(l);

  load_d(l);

  // Get a location which is a L_DREG
  l2= cgalloclocn(L_DREG, cgprimtype(newtype), NULL, 0);

  // Three possibilities: size 1 to 2, size 2 to 4 and size 1 to 4.
  // Note that chars are unsigned which makes things easier
  switch(how) {
      // 1 to 2
    case 1: fprintf(Outfile, "\tclra\n"); break;

      // 2 to 4
    case 2:
      // Get two labels
      label1 = genlabel();
      label2 = genlabel();
      fprintf(Outfile, "\tbge L%d\n", label1);
      fprintf(Outfile, "\tldy #65535\n");
      fprintf(Outfile, "\tbra L%d\n", label2);
      cglabel(label1);
      fprintf(Outfile, "\tldy #0\n");
      cglabel(label2);
      break;

      // 1 to 4
    case 3:
      fprintf(Outfile, "\tclra\n");
      fprintf(Outfile, "\tldy #0\n");
  }

  return (l2);
}

// Change a location from its old type to a new type.
int cgcast(int l, int oldtype, int newtype) {
  return(cgwiden(l,oldtype,newtype));
}

// Generate code to return a value from a function
void cgreturn(int l, struct symtable *sym) {
  // Load D is there is a return value
  if (l != NOREG)
    load_d(l);
  cgjump(sym->st_endlabel);
}

// Generate code to load the address of an identifier.
// Return a new location
int cgaddress(struct symtable *sym) {
  int l;

  // For things not on the stack it's easy
  if (sym->class == V_GLOBAL ||
      sym->class == V_EXTERN || sym->class == V_STATIC) {
    l= cgalloclocn(L_SYMADDR, PR_POINTER, sym->name, 0);
    return(l);
  }

  // For things on the stack we need to get the address in
  // the X register and then move it into D. Stash D in
  // a temporary if it's already in use.
  stash_d();
  fprintf(Outfile, "\tleax %d,s\n", sym->st_posn + sp_adjust);
  fprintf(Outfile, "\ttfr x,d\n");
  l = cgalloclocn(L_DREG, PR_POINTER, NULL, 0);
  return(l);
}

// Dereference a pointer to get the value
// it points at into the same location
int cgderef(int l, int type) {
  // Get the type that we are pointing to
  int newtype = value_at(type);
  int primtype= cgprimtype(newtype);

  if (Locn[l].type==L_DREG)
    fprintf(Outfile, "\ttfr d,x\n");
  else {
    // Stash D in a temporary if it's already in use.
    stash_d();
    fprintf(Outfile, "\tldx "); printlocation(l, 0, 'd');
  }

  switch (primtype) {
  case PR_CHAR:
    fprintf(Outfile, "\tldb 0,x\n"); break;
  case PR_INT:
  case PR_POINTER:
    fprintf(Outfile, "\tldd 0,x\n"); break;
  case PR_LONG:
    fprintf(Outfile, "\tldd 2,x\n");
    fprintf(Outfile, "\tldy 0,x\n");
  }

  cgfreelocn(l);
  l= cgalloclocn(L_DREG, primtype, NULL, 0);
  return (l);
}

// Dereference and store through l2, a pointer
int cgstorderef(int l1, int l2, int type) {
  int primtype = cgprimtype(type);

  // If l2 is in the D register, do a transfer
  if (d_holds== l2) {
    fprintf(Outfile, "\ttfr d,x\n");
  } else {
    fprintf(Outfile, "\tldx "); printlocation(l2, 0, 'd');
  }

  d_holds= NOREG;
  load_d(l1);

  switch (primtype) {
  case PR_CHAR:
    fprintf(Outfile, "\tstb 0,x\n"); break;
  case PR_INT:
  case PR_POINTER:
    fprintf(Outfile, "\tstd 0,x\n"); break;
  case PR_LONG:
    fprintf(Outfile, "\tsty 0,x\n");
    fprintf(Outfile, "\tstd 2,x\n"); break;
  }

  d_holds= l1;
  return (11);
}

// Generate a switch jump table and the code to
// load the locations and call the switch() code
void cgswitch(int reg, int casecount, int toplabel,
	      int *caselabel, int *caseval, int defaultlabel) {
  int i, label;

  // Get a label for the switch jump table
  label= genlabel();

  // Generate the switch jump table.
  cglitseg();
  cglabel(label);
  fprintf(Outfile, "\t.word %d\n", casecount);
  for (i = 0; i < casecount; i++)
    fprintf(Outfile, "\t.word %d\n\t.word L%d\n", caseval[i], caselabel[i]);
  fprintf(Outfile, "\t.word L%d\n", defaultlabel);

  // Output the label where we restart actual code
  cgtextseg();
  cglabel(toplabel);

  // Call the helper routine with the jump table location
  // after loading D with the value in reg (a location)
  load_d(reg);
  fprintf(Outfile, "\tldx #L%d\n", label);
  fprintf(Outfile, "\tbra __switch\n");
}

// Move value between locations
void cgmove(int l1, int l2, int type) {

  load_d(l1);
  save_d(l2);
}

// Output a gdb directive to say on which
// source code line number the following
// assembly code came from
void cglinenum(int line) {
  fprintf(Outfile, ";\t\t\t\t\tline %d\n", line);
}
