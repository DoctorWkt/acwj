## Thu 16 May 2024 10:41:14 AEST

A start on cwj for the 6809. I'm using a simple approach:
use memory locations as the registers. Later on, I'll
improve the code quality.

I also need to split the compiler into phases. One to
parse and make the AST and the symbol table. The other
to output the assembly from the AST and symbol table.

Hopefully I can get it all to fit into 64K!

Right now I can do:

```
int p=3; int q=4;
int main() { p= p + q; return(0); }
```

## Thu 16 May 2024 14:55:39 AEST

I added the code to push arguments and fix up
the stack after a function call. I can call
`printint()` which work. However, my compiler
isn't adding the leading underscore. To fix.

## Thu 16 May 2024 15:13:22 AEST

Fixed the `_` by adding it to all the `fprintf()`s
in `cg.c`.

## Thu 16 May 2024 16:49:54 AEST

Hmm. Small int lits are treated as char lits, but
I need them to be int sized for `printf()`. 

Ah. I already did this when I last tried to write
a 6809 compiler using `cwj`, see
cloud/Nine_E/Old/Compiler` dated June 2023!

I've imported some of that code which has helped.

## Sat 18 May 2024 10:00:44 AEST

OK I've finished the conversion of `cg.c` to 6809 but
no testing yet. Just about to change over to the `crt0`
which has stdio suuport, because all the tests use
printf. I'm using the `libc` compiled by `fcc` for now.

## Sat 18 May 2024 13:44:47 AEST

Not quite finished. I'd forgotten the comparisons.
I've done them except for longs. Now up to
input009.c: OK.

## Sat 18 May 2024 14:33:42 AEST

Now up to input022.c: OK.

## Sat 18 May 2024 15:11:34 AEST

Now up to input054.c: OK

## Sat 18 May 2024 15:24:25 AEST

Now up to input114.c: OK
Wow!

## Mon 20 May 2024 10:42:50 AEST

Test 115 was a sizeof test which is now different on the 6809
cf. amd64 :-) So now we are up to input135.c: OK.

Test 136 is essentially this:

```
result= 3 * add(2,3) - 5 * add(4,6);
```

I've checked the debug output and the `add()` is working fine.
However, on the first `add()` return I see:

```
       lbsr _add
        leas 4,s
        puls d
        std     R0
        std     R1
```

which doesn't make sense as `R0` has the 3 from the `3*`.
I see the problem. We push any in-use local registers on
the stack before a function call. On return, we pop them
off and restore them. Then we save the function's return
value.

But the function uses `Y,D` to hold the return value,
and these are getting destroyed by the register popping.

OK fixed with a slightly ugly fix. Now up to
input143.c OK.

## Mon 20 May 2024 12:33:02 AEST

Yay. I now pass all the tests :-) I had to import some
stuff from the Fuzix include files into my include files.
That means I can now start on breaking the compiler up
into phases.

There are going to be eight phases.

1. The C pre-processor interprets #include, #ifdef
   and the pre-processor macros.
2. The lexer reads this and produces a token stream.
3. The parser reads the token stream and creates
   a symbol table plus a set of AST trees.
4. The AST optimiser reads the trees and optimises them.
5. The code generator reads the new AST trees and
   the symbol table, and generates assembly code.
6. The peephole optimiser improves the assembly code.
7. The assembler produces object files.
8. The linker takes crt0.o, the object files and
   several libraries and produces an executable.

As well, there will be debug tools to:
 - dump the token stream,
 - dump the AST trees, and
 - dump the symbol table

For the symbol table, it will be in a file that
has the globals, prototypes, struct/union/typedef
defines. Then followed by sections that have the
per-function symbols.

The AST file will have several separate AST trees,
one for each function.

I already did some of this when I was working on
PipeC, so I can borrow this code. Yay!

## Mon 20 May 2024 14:55:17 AEST

I've got separate scanner and detok programs with
the code borrowed from PipeC. I also added tokens
that hold the new filename and new linenumber when
these change. The weird thing is that, from this
input:

```
# 0 "scan.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "scan.c"
# 1 "defs.h" 1
# 1 "/tmp/include/stdlib.h" 1 3 4




# 4 "/tmp/include/stdlib.h" 3 4
void exit(int status);
...
```

I get these tokens:

```
1E: void
43: filename "/tmp/include/stdlib.h"
44: linenum 4
36: exit
39: (
20: int
36: status
3A: )
35: ;
```

with the `void` coming before the filename!
It's because of the `scan()` recursion. I've
got it fixed now. The line numbers do seem to
be a bit out, though.

## Tue 21 May 2024 09:57:34 AEST

So, I've rebuilt the compiler with a front-end which
reads the token stream from stdin and calls
`global_declarations()` to start the compilation. Right
now it outputs mostly the same assembly code, except for:

```
input021.s has changed in the tree
input058.s has changed in the tree
input084.s has changed in the tree
input089.s has changed in the tree
input090.s has changed in the tree
input134.s has changed in the tree
input140.s has changed in the tree
input148.s has changed in the tree
```

So now I've got some stuff to look at!
Ah, I'd forgotten the CHARLITs in the
token decoder. Fixed. Now the compiler
produces the same assembly files for
all tests. Yay!

## Tue 21 May 2024 11:08:53 AEST

I'm now working on the parsing phase.
I've got the code now to serialise
all the ASTs out to stdout and there is
a `detree` to print the trees.

There's a small problem that we used to
generate assembly for string literals
along with labels while parsing. I've
commented this out for now but it will
need fixing.

Now I need to dink `gen.c` out from
the parser, and also work out to
dump the symbol table!

## Tue 21 May 2024 11:24:39 AEST

I've pulled `gen.c` and `cg.c` out
from the parser. I had to abstract out
a few functions from these files into a
new `target.c` file which can be shared
by the parser and code generator.

Now to think about the symbol table serialisation.

I'm worried that we might do:

```
<global variable>
<function1>
<global variable>
<function2>
```

If we dump out the global symbol table before
`function1` then we won't have the second
global variable.

Can I just seralise symbols as they get declared?
No as they get modified e.g. an array with declared
elements. Perhaps I keep pointers to the last symbols
dumped, and dump from there each time we complete a
function?

## Tue 21 May 2024 16:53:06 AEST

OK, I've got the start for the serialisation code.
Now need to write the deserialiser and see where
the bugs are.

## Wed 22 May 2024 08:28:42 AEST

I rewrote the serialising code a bit. It runs and
doesn't crash. Now to write the deserialiser!

OK, I've got a start so I can see what's in the symbol file.
Still not right as yet.

Found a couple of bugs and fixed them. Right now I'm only
dumping the symbols, I'm not rebuilding the symbol table.
That will require a new program, the code generator. This
will do the symbol table and AST deserialising, and have:

```
gen.c misc.c sym.c tree.c type.c cg.c
```

## Wed 22 May 2024 15:26:54 AEST

I now have a code generator and it's actually working. I
had to fix a bunch of things like storing string lits in
the AST so I could generate them later. But I'm now passing
the first dozen tests! Yay!

We are now up to input025.c: OK. Wow. Test 26 is failing
because none of the params or locals have a frame offset.

## Wed 22 May 2024 16:42:47 AEST

Actually they do, but I had a bug in my find symbol by id code.
We are now up to input057.c OK.

## Thu 23 May 2024 10:17:43 AEST

Ah, I was serialising the globals first in the symbol table
but they may be of struct type, so I need to output the
structs, enums and unions before the globals. Now up to
input073.c: OK.

That was because I switched to the literal segment from the
text segment, and went to the data segment not back to the
code segment! Now up to input088.c: OK.

So the problem is:

```
char *z= "Hello world";
```

In the original code, we could generate the string literal, get
a label for it and then generate the global `z` with the label.
Now the string gets lost and `z` is initialised to zero. We also
have to deal with `char *z=NULL;` also.

I was thinking of putting the characters of the string in the
symbol's `initlist` and setting `nelems` to the string length,
but what if the initial value is NULL? Can I set `nelems` to
zero? But what if we do `char *z= "";`, so not NULL but no
characters? Hmm ... No that's not an answer.

And we also need to support:

```
char *fred[]= { "Hello", "there", "Warren", "piano", NULL };
```

which the original `cwj` compiler supported. Maybe I need another
symbol table for string literals. Then I can serialise that and
generate the strings and labels in the back end?

## Fri 24 May 2024 06:32:06 AEST

My solution will be the string literal symbol tables. The initlist
for `char *` globals will have the symid dumped in the symfile.
In the generator, we load the symbol tables. We generate the asm code
for the string literals & make a label. Then when we hit a `char *`
global we replace the symids in the initlist with the relevant labels.

## Fri 24 May 2024 09:42:04 AEST

OK. Implemented. Seems to work. We are now up to input098.c: OK.
Actually, I forgot about arrays of strings. Now fixed and we are
up to input129.c: OK.

Looks like we are not dealing with this string literal as an
expression:

```
"Hello " "world" "\n"
```

## Sat 25 May 2024 07:49:15 AEST

Ah yes, I wasn't incrementing `litlast` correctly in `primary()`,
now fixed. And now we pass all the tests! Some of the error reporting
isn't exactly right, but that's fine. Yay, we have reached a milestone :-)

I was thinking of bringing the QBE backend back in as well. Then we will
have two backends which might help reveal more bugs. Hopefully it won't
be too hard to adapt the existing backend to suit the rearranged compiler.
And I also have to write the frontend which I'll call `wcc.c` to run
all the phases correctly.

So here's a TODO list:

 - create the QBE backend, get it to work correctly
 - make it possible to build the compiler with each backend
 - change the final location to be `/opt/wcc` a la `fcc`
 - tease out the tree optimisation as an other phase, and
   add some of the `SubC` optimisations
 - bring in the peephole optimiser from `fcc` and make it a phase
 - improve the 6809 code generation
 - add in `register, volatile, unsigned, float, double, const`
 - Eventually, start compiling the 6809 libc and see what other
   language features I need to add.

This is going to take quite a while!

## Sat 25 May 2024 08:30:19 AEST

I've rearranged the filenames and massaged the Makefile so I now
have 6809-specific executables for "parse" and "gen". I imported
the QBE backend. The old compiler used to pass the type down into
the `cg` functions, whereas the new compiler doesn't do this. I
think I can go back to the old way, as it would help with other
backends apart from 6809 and QBE.

OK, I've nearly everything. The new code pushes the function args
with one `cg` function and does the call with another. The existing
QBE backend does it in one function. So I'll need to split that up.
But that's the only thing left to do, apart from testing it :-)

Actually the reason it changed for QBE is that QBE does the call
first and the list of args second. I guess I could change the 6809
version to use this.

## Sat 25 May 2024 17:14:01 AEST

I've done the change and fixed a bug or two along the way. We 
are up to test input135.c: OK, so it's very close!

All tests now pass, I'd forgotten about spilling registers before
calling a function.

## Sun 26 May 2024 08:09:25 AEST

I'm working on `cgqbe.c` now. Almost done, I need to write `cgswitch()`
in the QBE file; it was in `gen.c` in the QBE version previously.

## Sun 26 May 2024 11:01:30 AEST

I've written the `cgswitch()` in `cgqbe.c` and also added code to
delay outputting string literals until after the code. We are up to
input010.c: OK. A small issue, now up to input057.c: OK.

## Sun 26 May 2024 11:42:03 AEST

The size of INTLITs used to access a struct member are longs in QBE,
ints on the 6809. Another target function added.

We are up to input062 which is the endianness test. I'll have to
remove it for now. Now up to input074.c which is the first switch
test and which is failing at present.

## Sun 26 May 2024 12:19:05 AEST

I've fixed up the bug in `cgqbe.c` and also rearranged the switch
generation code. Now all tests pass on both the 6809 and the QBE side.
Yay, I now have a compiler with two back ends!!!

## Tue 28 May 2024 09:23:11 AEST

I've written the front-end `wcc` and it now works for both QBE
and 6809. I've moved install to `/opt/wcc`. I've rewritten
`runtests` and `onetest`. Right now, for some reason, one of the
QBE tests is failing. Sigh!

## Tue 28 May 2024 10:34:14 AEST

Ah, there are several things in `include` which are different
between 6809 and QBE. So we now have two `include` trees, one
for each platform. We now pass the tests again!

## Tue 28 May 2024 11:12:20 AEST

I just got the peephole optimiser added to the front-end. My
first rule failed! Wonder why. Ah, I'd written the rule wrong, fixed.
The 6809 tests pass with the peephole optimiser working.

## Tue 28 May 2024 11:50:54 AEST

I teased out the AST optimiser as a standalone program, leaving it
still in the parser. It seems to work. However, I then tried to
remove the optimiser from the parser with problems: We have this:

```
decl.c:  // Parse the expression and optimise the resulting AST tree
decl.c:  tree = optimise(binexpr(0));
```

With this line changed to `tree=binexpr(0);` instead, we die on
test 112 with `Cannot initialise globals with a general expression`. Why:

```
int x= 10 + 6;
```

These need to be resolved at parse time. Argh! Perhaps I can keep
some of the tree optimisation code in the parser? Don't know.

Just looking at the current amd64 binary sizes:

```
   text	   data	    bss	    dec	    hex	filename
   9973	   1008	   4200	  15181	   3b4d	wcc
   8902	    676	    680	  10258	   2812	cscan
  13695	    752	   1328	  15775	   3d9f	cpeep
  38772	   1880	    936	  41588	   a274	cparse6809
  47917	   1456	   1064	  50437	   c505	cgen6809
  38796	   1880	    936	  41612	   a28c	cparseqbe
  35088	   1264	    968	  37320	   91c8	cgenqbe
```

It looks like the 6809 generator needs some dieting before the parser does.

I decided to try compiling the compiler code itself with the 6809 compiler:

```
$ for i in *.c; do wcc -c -m 6809 $i; done
Expecting a primary expression, got token:void on line 27 of cgen.c
Type mismatch: literal vs. variable on line 20 of cgqbe.c
Unrecognised character \
Unrecognised character \
Unrecognised character \
Unrecognised character \
Unrecognised character \
Type mismatch: literal vs. variable on line 35 of cpeep.c
Expecting a primary expression, got token:void on line 23 of ctreeopt.c
Expecting a primary expression, got token:void on line 25 of desym.c
Unrecognised character \
Unrecognised character \
Expecting a primary expression, got token:void on line 22 of detok.c
Expecting a primary expression, got token:void on line 25 of detree.c
Expecting a primary expression, got token:void on line 27 of parse.c
& operator must be followed by an identifier on line 604 of scan.c
Unrecognised character \
Unknown variable or function:s on line 143 of tree.c
Expecting a primary expression, got token:} on line 17 of wcc.h

$ ls *.o
 71859 May 28 13:30 cg6809.o
   688 May 28 13:26 crt0.o
 33009 May 28 13:30 decl.o
 29981 May 28 13:30 expr.o
 22461 May 28 13:30 gen.o
  1342 May 28 13:30 misc.o
  3639 May 28 13:30 opt.o
 12545 May 28 13:30 stmt.o
 22795 May 28 13:30 sym.o
  1053 May 28 13:30 targ6809.o
  1393 May 28 13:30 targqbe.o
   126 May 28 13:30 tstring.o
  6732 May 28 13:30 types.o
```

Interesting. I need to find out why the scanner/parser is dieing.
Also `cg6809.o` is way too big!

## Tue 28 May 2024 14:03:18 AEST

I fixed the bug where the scanner could not deal with `\"` inside
string literals. We now have:

```
Expecting a primary expression, got token:void on line 27 of cgen.c
Type mismatch: literal vs. variable on line 20 of cgqbe.c
Type mismatch: literal vs. variable on line 35 of cpeep.c
Expecting a primary expression, got token:void on line 23 of ctreeopt.c
Expecting a primary expression, got token:void on line 25 of desym.c
Expecting a primary expression, got token:void on line 22 of detok.c
Expecting a primary expression, got token:void on line 25 of detree.c
Expecting a primary expression, got token:void on line 27 of parse.c
& operator must be followed by an identifier on line 612 of scan.c
Expecting a primary expression, got token:} on line 17 of wcc.h
```

The problem is that the line numbers are a bit bogus :-(

Ah, the bug is that my compiler doesn't allow `return <expression>;`,
it only likes `return(<expression>);`.

## Tue 28 May 2024 14:26:43 AEST

I tried to fix it but it's hard. We have to be able to deal with:

```
    return( (void *)0 );
and
    return (void *)0 ;
```

For now I'll just fix my own code. Done.

## Tue 28 May 2024 14:47:59 AEST

The `& operator must be followed by an identifier` is because we
can't do this yet:

```
mary= &fred.x;
```

OK, down to these:

```
$ for i in *.c; do wcc -S -m6809 $i; done
Type mismatch: literal vs. variable on line 20 of cgqbe.c
Type mismatch: literal vs. variable on line 35 of cpeep.c
```

## Tue 28 May 2024 15:14:56 AEST

I'm down to one source file now:

```
$ for i in *.c; do wcc -c -m6809 $i; done
Incompatible types in binary expression on line 95 of cpeep.c

$ ls *.o
 71859 May 28 15:14 cg6809.o
 12497 May 28 15:14 cgen.o
 37028 May 28 15:14 cgqbe.o
   688 May 28 15:13 crt0.o
  8427 May 28 15:14 ctreeopt.o
 33009 May 28 15:14 decl.o
  4832 May 28 15:14 desym.o
  3093 May 28 15:14 detok.o
  3336 May 28 15:14 detree.o
 29981 May 28 15:14 expr.o
 22461 May 28 15:14 gen.o
  1342 May 28 15:14 misc.o
  3639 May 28 15:14 opt.o
  8853 May 28 15:14 parse.o
 23115 May 28 15:14 scan.o
 12545 May 28 15:14 stmt.o
 22795 May 28 15:14 sym.o
  1053 May 28 15:14 targ6809.o
  1393 May 28 15:14 targqbe.o
  9685 May 28 15:14 tree.o
   126 May 28 15:14 tstring.o
  6732 May 28 15:14 types.o
 19125 May 28 15:14 wcc.o
```

I decided to get `fcc` to compile the code. The results are:

```
$ ls *.o
 35320 May 28 15:21 cg6809.o
  5587 May 28 15:21 cgen.o
 16838 May 28 15:21 cgqbe.o
 17642 May 28 15:21 cpeep.o
  3965 May 28 15:21 ctreeopt.o
 14467 May 28 15:21 decl.o
  2308 May 28 15:21 desym.o
  1569 May 28 15:21 detok.o
  1520 May 28 15:21 detree.o
 11534 May 28 15:21 expr.o
  8617 May 28 15:21 gen.o
   742 May 28 15:21 misc.o
  1521 May 28 15:21 opt.o
  4228 May 28 15:21 parse.o
 11414 May 28 15:21 scan.o
  5617 May 28 15:21 stmt.o
 10648 May 28 15:21 sym.o
   761 May 28 15:21 targ6809.o
   903 May 28 15:21 targqbe.o
  5152 May 28 15:21 tree.o
  2012 May 28 15:21 tstring.o
  2515 May 28 15:21 types.o
 10225 May 28 15:21 wcc.o
```

or about half the size :-)

OK, just for fun I built `cgen6809` using the `fcc` compiler.
I get `8A91 B __end` as the size. And for `cparse6809` we have
`765A B __end` :-)

And the amd64 versions have these size:

```
$ size cparse6809 cgen6809
   text	   data	    bss	    dec	    hex	filename
  38793	   1880	    936	  41609	   a289	cparse6809
  47942	   1456	   1064	  50462	   c51e	cgen6809
```

So, if we can get the code generator to be around as good as `fcc`
then we stand a chance of getting it to cross-compile. Lots of
work to do.

## Tue 28 May 2024 16:34:46 AEST

Just brainstorming the code improvement. We sort of have to keep
the register idea in `gen.c` so as to support QBE. How about:

 - cg6809.c keeps an array of Location structs
 - the index into the array is a "register"
 - global `d_free` so we know when we can load the D register
 - the Location holds enough details to use as an operand
   to the B/D/Y operations.

Looking at the current code:

```
ldd #0\n");
ldd 0,x\n");
ldd #1\n");
ldd 2,x\n");
ldd #%d\n", offset);
ldd #%d\n", offset & 0xffff);
ldd #%d\n", val & 0xff);
ldd #%d\n", value & 0xffff);
ldd #%d\n", (value>>16) & 0xffff);
ldd %d,u\n", 2+sym->st_posn);
ldd %d,u\n", sym->st_posn);
ldd #L%d\n", label);
ldd _%s+2\n", sym->name);
ldd #_%s\n", sym->name);
ldd _%s\n", sym->name);
```

so we need to record:

 - symbol names with optional position
 - offset on the stack frame
 - constants
 - label-ids
 - address of symbol names

Something like:

```
enum {
  L_SYMBOL,
  L_LOCAL,
  L_CONST,
  L_LABEL,
  L_SYMADDR,
  L_DREG
};

struct Location {
  int type;
  char *name;
  int intval;		// Offset, const value, label-id
};
```
and a function which prints out the Location. We keep the
register allocation/freeing and we can set `d_free` true
when we free all registers. Register spilling should be
simpler. The `cg` functions which allocate a register
will now allocate a Location element and fill it in.

Then, something like:

```
int cgadd(int r1, int r2, int type) {
  int size= cgprimsize(type);

  // If r1 is already L_DREG, do nothing.
  // Otherwise load the existing r1 location
  // into D and mark it as L_DREG.
  // This could load B, D or Y,D
  load_d(r1, size);	

  switch (size) {
    case 1: fprintf(Outfile, "\taddb"); printlocation(r2,0); break;
    case 2: fprintf(Outfile, "\taddd"); printlocation(r2,0); break;
    case 4: fprintf(Outfile, "\taddd"); printlocation(r2,2);
	    // Some code here to update Y :-)
  }
  return(r1);
}
```

## Wed 29 May 2024 09:01:58 AEST

I got a start last night with good results. I've thought of some
improvements and will try to get some done now.

I can compile this so far:

```
int x, y, z;

int main() {
  int result; x=2; y=3; z=4;
  result= x + y + z; printf("%d\n", result); return(result);
}
```

with the assembly (some bits omitted):

```
_main:
	pshs u
	tfr s,u
	leas -2,s
	ldd #2
	std _x
	ldd #3
	std _y
	ldd #4
	std _z
	ldd _x+0
	addd _y+0
	addd _z+0
	std -2,u

	ldd -2,u	; This could be improved!
	pshs d
	ldd #L2
	pshs d
	lbsr _printf
	leas 4,s

	ldd -2,u
	leas 2,s
	puls u
	rts
```

which is much nicer than going through R0, R1 etc.

## Wed 29 May 2024 12:48:15 AEST

It's slow going. We are up to input009.c: OK though.
Now input010.c: OK.

I can see that dealing with longs isn't going to be fun.

## Thu 30 May 2024 12:51:23 AEST

Fait went away with Liz today :-(
We are up to input026.c: OK

## Thu 30 May 2024 13:19:48 AEST

Now up to input090.c OK.

## Thu 30 May 2024 14:05:55 AEST

Now up to input139.c: OK

## Thu 30 May 2024 15:09:40 AEST

Yay, all the tests now pass. Wow.

For a lark I tried to compile the compiler source with itself:

```
$ for i in *.c; do wcc -c -m6809 $i; done
Incompatible argument type in function call on line 58 of cg6809.c
Out of locations in cgalloclocn on line 241 of (null)
Incompatible types in binary expression on line 95 of cpeep.c
Out of locations in cgalloclocn on line 284 of (null)
Out of locations in cgalloclocn on line 41 of (null)
child phase didn't Exit
Out of locations in cgalloclocn on line 69 of (null)
child phase didn't Exit
Out of locations in cgalloclocn on line 48 of (null)
```

So a few files didn't compile, but for those here are
the size changes from the old to new code generator:

```
Old Size              New Size		Fcc Size
------------------------------------------------
 71859 cg6809.o
 12497 cgen.o		7890		5587
 37028 cgqbe.o
   688 crt0.o
  8427 ctreeopt.o	5530		3965
 33009 decl.o
  4832 desym.o		3464		2308
  3093 detok.o		2242		1569
  3336 detree.o		2313		1520
 29981 expr.o
 22461 gen.o		11608		8617
  1342 misc.o		834		742
  3639 opt.o		2132		1521
  8853 parse.o		5897		4228
 23115 scan.o
 12545 stmt.o
 22795 sym.o
  1053 targ6809.o	806		761
  1393 targqbe.o	1062		903
  9685 tree.o
   126 tstring.o	126		2012
  6732 types.o		4385		2515
 19125 wcc.o		13238		10225
```

Quite an improvement I think. And there's more work to do as `fcc`
is still much better.

## Thu 30 May 2024 17:43:35 AEST

Found a bug in `gen.c` because pointers and ints are different
sizes in QBE/amd64. This line `Locn[l].type = 23;` failed because
`l` was being multiplied by the struct size (as an int) but then
was added to the address of `Locn` (long vs. word). Fixed.

No, not fixed. It trips up later when it tries to widen a long
to be a long, which QBE can't do (for some reason :-).

## Thu 30 May 2024 20:02:32 AEST

Fixed and fixed a few others. We now have:

```
$ for i in *.c; do wcc -c $i; done
Incompatible types in binary expression on line 95 of cpeep.c
qbe:decl.c_qbe:2530: invalid type for first operand %class in copy
```

The `cpeep.c` one is subtracting pointers and assigning to an int:

```
int main() {
  char *a, *b;
  int x;
  x= a - b;
  return(x);
}
```

The other one seems to be that `class` should be marked as having
an address but it isn't. Not sure why not.

## Fri 31 May 2024 08:56:58 AEST

Ideas for putting the compiler on a diet.

 - More AST optimisations, e.g. add/sub/mul where one side is int lit 0.
   But use an #ifdef to keep the gen ones away from the parse ones.
   Also e.g. switch left/right on commutative ops so that D reg already
   holds one of the values.
 - Use free() where possible
 - When an AST op is the top of the tree, don't load the D register with
   the result, e.g. `i++;`.
 - Don't use variables, e.g. `int primtype= ...; switch(primtype)`
 - What ints can be turned into chars?
 - We need to keep P_POINTER even though it's the same as P_INT at
   present. When we have `unsigned` then pointers will be unsigned,
   but ints might be signed.
 - Find duplicated string lits and make them globals, so only declared once.
 - Definitely some 6809 code improvements.
 - More peephole optimisations
 - Do a code coverage analysis?
 - Move the temporaries on the stack. Use an assembler constant to `leas`
   the stack at the start/end of each function. This will save us the
   necessity of spilling temps on the stack.

## Fri 31 May 2024 09:22:30 AEST

So we need to be able to add/subtract pointers. But we have to unscale
the result. Example (on amd64 gcc):

```
#include <stdio.h>
int main() {
  int a, b; int *x, *y; long z;
  a=2; b=3; x=&a; y=&b;
  printf("x is %lx y is %lx\n", x, y);
  z= x - y;     printf("z is %lx\n", z);
  z= x - y + 1; printf("z is %lx\n", z);
  return(0);
}
```

produces

```
x is 7ffc7b25b244 y is 7ffc7b25b240
z is 1
z is 2
```

Note x and y are four bytes apart, but the subtraction gives 1 as a result.
But then the `+1` is treated as a long addition. Hmm.

Yes I think we will need an A_DESCALE AST operation and a `cgshrconst()`
function.

## Fri 31 May 2024 14:43:58 AEST

Looking at the QBE `%class` problem in `decl.c`. I've got a new file `d.c`
with just the problem function `declaration_list()` in it. This compiles
with no problem! I even put in a function prototype just like in `decl.c`
with no problems. Hmm.

In the `sym` file, this is `class`:

```
{name = 0x555555566860 "class", id = 575, type = 48, ctype = 0x0, 
  ctypeid = 0, stype = 0, class = 3, size = 4, nelems = 1, st_hasaddr = 0, 
  st_posn = 0, initlist = 0x0, next = 0x555555566880, member = 0x0}
```

and `st_hasaddr` is zero. But I ran the parser through `gdb` and saw
it being set to 1. So not sure how it got reset to 0.

I ran the parser again. No, nothing is resetting it to 0. I modified
`desym` to show `hasaddr` and none of the parameters have it set to 1.

I added a printf to the parser and I see:

```
In declaration_list set hasaddr 1 on class stype 0 class 3
```

which is a variable (0) parameter (3). Hmm. So why isn't it being
dumped properly?

## Sat 01 Jun 2024 10:03:16 AEST

OK I think I can see the problem. The symbol is being dumped before
`hasaddr` is set:

```
$ wcc -S -X -v decl.c
...
Serialising class stype 0 class 3 id 575 hasaddr 0
...
In declaration_list set hasaddr 1 on class stype 0 class 3 id 575
```

I'm guessing that we serialise the `class` mentioned in the prototype
when it has `hasaddr 0`. Later on, `hasaddr` gets set but as the
symbol was already serialised, the change doesn't make it out. Damn.

We have to output protptypes in the symbol table in case a function
calls a function which is only represented by the prototype.

My solution for now is to mark all parameters as having an address
in `cgqbe.c`. I only have to do the parameters as they appear in
the prototypes. Sigh. All the tests still pass. Now we are down to:

```
Incompatible types in binary expression on line 95 of cpeep.c
```

But as I don't use this in the QBE version of the compiler, I should
be able to try and compile the QBE compiler with itself.

## Sat 01 Jun 2024 13:26:27 AEST

The `wcc` front-end doesn't work but I suspect system calls. `cscan`
works fine.

Damn, the parser isn't right, I'm seeing:

```
$ ls *ast ; ls *sym
 235105 Jun  1 13:28 decl.c_ast
 216017 Jun  1 13:28 fred_ast
 59119 Jun  1 13:28 decl.c_sym
 56399 Jun  1 13:28 fred_sym
```

as the outputs, the "fred" ones are from `cparseqbe` compiled with `wcc`.

Ah, it looks like the struct sizes are different. I've run `detree`
(compiled with wcc and gcc) and I see these differences:

```
<       STRLIT rval \"Can't have static/extern in a typedef declaration\"
---
>       STRLIT rval "Can't have static/extern in a typedef declaration"
1641c1641
<       STRLIT rval \"redefinition of typedef\"
---
>       STRLIT rval "redefinition of typedef"
1669c1669
<       STRLIT rval \"unknown type\"
---
>       STRLIT rval "unknown type"
...
```

Not sure if that's bad or not. For the dumped symbol table, `desymqbe`
produces the same output except the `wcc` compiled one does a segfault
right at the end. Sigh.

Damn, I made the `.s` files for `desymqbe` and did a
`cc -o desymqbe -g -Wall *.s` but that didn't tell me where it crashed.

Hmm, well I can actually do this:

```
$ ./wscan < decl.c_cpp > fred.tok
$ ./wparseqbe fred_sym fred_ast < fred.tok
$ ./wgenqbe fred_sym fred_ast > fred_qbe
```

with no crashes. Doing a diff:

```
$ paste fred_qbe decl.c_qbe

@L2     		  @L2
  %ctype =l alloc8 1      %ctype =l alloc8 1
  storel %.pctype, %ctype storel %.pctype, %ctype
  %class =l alloc8 1      %class =l alloc8 1
  storel %.pclass, %class storel %.pclass, %class
  %.t1 =w copy 0          %.t1 =w copy 0
  %type =w copy %.t1      %type =w copy %.t1
  %.t2 =w copy 1          %.t2 =w copy 1
  %exstatic =w copy %.t2  %exstatic =w copy %.t2
  %.t3 =w copy 0          %.t3 =w copy 0
  %.t5 =l extsw %.t3      %.t5 =l extsw %.t3
  %.t6 =l loadl $ctype    %.t6 =l loadl %ctype	<===
  storel %.t5, %.t6       storel %.t5, %.t6
```

`ctype` is a local and should always be `%ctype`,
but our compiler is producing `$ctype` which is
a global. Hmm ...

## Sat 01 Jun 2024 15:23:22 AEST

Going back to `desymqbe`, I added a few printfs ...

```
int main() {
  struct symtable sym;

  while (1) {
printf("A\n");
    if (deserialiseSym(&sym, stdin)== -1) break;
printf("B\n");
    dumpsym(&sym, 0);
printf("C\n");
  }
printf("D\n");
  return(0);
}
```

and when I run the version compiled with our compiler:

```
C
A
D
Segmentation fault
```

So it's segfaulting on the `return(0)`? When I change it
to an `exit(0)` it's fine?! I'll do that for now.

## Sat 01 Jun 2024 15:29:20 AEST

Summary: we can link and run the passes but not the front-end `wcc`.
Doing a compile of `decl.c` to assembly:

- the token files are identical
- the dumped symbol files are identical from the `desym` perspective
- the strlits in the AST file are different. The self-compiled version
  is putting \ before each "
- the `cgenqbe` seems to be treating locals as globals here and there.

## Sun 02 Jun 2024 09:42:19 AEST

I moved the symbol dumping code from `sym.c` to `desym.c` and
added more code to print out everything.

I need a name for the two sets of binaries. The G binaries are compiled
with `gcc`, the W binaries with `wcc`.

The G & W tokeniser produce the same token stream.
The G & W parser produce different symbol table files,
but when I run the G & W `desym` on them I get identical results.

We still have the STRLIT issue with the G & W `detree` outputs.
The AST files are different, but doing a `hd` on them I see:

```
G version
00030440  00 00 00 00 44 75 70 6c  69 63 61 74 65 20 73 74  |....Duplicate st|
00030450  72 75 63 74 2f 75 6e 69  6f 6e 20 6d 65 6d 62 65  |ruct/union membe|
00030460  72 20 64 65 63 6c 61 72  61 74 69 6f 6e 00 1c 00  |r declaration...|

W version
00034880  00 00 00 00 44 75 70 6c  69 63 61 74 65 20 73 74  |....Duplicate st|
00034890  72 75 63 74 2f 75 6e 69  6f 6e 20 6d 65 6d 62 65  |ruct/union membe|
000348a0  72 20 64 65 63 6c 61 72  61 74 69 6f 6e 00 1c 00  |r declaration...|
```

so the literals are the same. It's just `detree` somehow printing out
them differently.

## Sun 02 Jun 2024 10:03:32 AEST

Hmm. So I added code in `cgqbe.c` to print out how we decide to use
either a local `%` or global `$` character. Yes, we get a wrong answer:

```
< loadvar ctype 3 -> %
< loadvar exstatic 2 -> %
---
> loadvar ctype 3 -> $
> loadvar exstatic 2 -> $

< loadvar class 3 -> %
< loadvar class 3 -> %
---
> loadvar class 3 -> $
> loadvar class 3 -> $
```

The code for this is:

```
  // Get the relevant QBE prefix for the symbol
  qbeprefix = ((sym->class == C_GLOBAL) || (sym->class == C_STATIC) ||
               (sym->class == C_EXTERN)) ? (char)'$' : (char)'%';
```

So it might be LOGOR or ternaries or a combination of both?!
I've got a test program and, for all class values we always get '$'
and not '%'. Hmm.

I'm comparing the QBE output from the `acwj 63` compiler. The LOGOR
code is the same. The ternary code isn't.

## Mon 03 Jun 2024 13:47:00 AEST

So it turns out I hadn't thought through the cases where a) I need
a boolean value from an arbitrary expression and b) when to compare
and jump, or) jump on a boolean value. The answer was as follows ...

We already have code in IF and WHILE statements, e.g.

```
  // Parse the following expression 
  // and the ')' following. Force a 
  // non-comparison operation to be boolean.
  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    condAST =
      mkastunary(A_TOBOOL, condAST->type, condAST->ctype, condAST, NULL, 0);
  rparen();
```

And in `gen.c` and `cg.c`:

```
  case A_TOBOOL:
    // If the parent AST node is an A_IF or A_WHILE, generate
    // a compare followed by a jump. Otherwise, set the register
    // to 0 or 1 based on it's zeroeness or non-zeroeness
    return (cgboolean(leftreg, parentASTop, iflabel, type));
...
  case A_EQ:
  case A_NE:
  case A_LT:
  case A_GT:
  case A_LE:
  case A_GE:
    // If the parent AST node is an A_IF, A_WHILE or A_TERNARY,
    // generate a compare followed by a jump. Otherwise, compare
    // registers and set one to 1 or 0 based on the comparison.
    if (parentASTop == A_IF || parentASTop == A_WHILE ||
        parentASTop == A_TERNARY)
      return (cgcompare_and_jump
              (n->op, leftreg, rightreg, iflabel, n->left->type));
    else
      return (cgcompare_and_set(n->op, leftreg, rightreg, n->left->type));
```

So, we can fix the problem by a) adding `mkastunary(A_TOBOOL...` to
the ternary expression code, and b) adding `parentASTop == A_TERNARY`
to the `case A_TOBOOL`. I've just done this and now all the tests pass
including a few extras I made up. Phew!

## Mon 03 Jun 2024 14:00:23 AEST

Now, going back to compiling `decl.c` with our own compiler, I now get:

```
$ md5sum decl.c_qbe fred.qbe
0f299257e088b3de96b68430e9d1f123  decl.c_qbe	G version
0f299257e088b3de96b68430e9d1f123  fred.qbe	W version
```

## Mon 03 Jun 2024 14:24:00 AEST

With a shell script that uses the W binaries to compile the passes down
to QBE code, I think I should be able to pass the triple test!

```
18fe9843b22b0ad06acfbc2011864619  cg6809.c_qbe	G version
18fe9843b22b0ad06acfbc2011864619  fred.qbe	W version
===
f7db53fe1cb35bd6ed19e633a5c618a6  cgen.c_qbe
f7db53fe1cb35bd6ed19e633a5c618a6  fred.qbe
===
9bfc78b1ef9b08eecf9dde1046bc4ab6  cgqbe.c_qbe
9bfc78b1ef9b08eecf9dde1046bc4ab6  fred.qbe
===
===
76ec1c53e8362a69ef1935a5efd7a1e3  ctreeopt.c_qbe
76ec1c53e8362a69ef1935a5efd7a1e3  fred.qbe
===
0f299257e088b3de96b68430e9d1f123  decl.c_qbe
0f299257e088b3de96b68430e9d1f123  fred.qbe
===
62d30bbd1b3efc61a021fffe76fff670  desym.c_qbe
62d30bbd1b3efc61a021fffe76fff670  fred.qbe
===
28e5d1e283ee25a6b7e08f1d69816de8  detok.c_qbe
28e5d1e283ee25a6b7e08f1d69816de8  fred.qbe
===
c1cdef1a287717a429281f6c439475d4  detree.c_qbe
c1cdef1a287717a429281f6c439475d4  fred.qbe
===
4c72e8a009e3e6730969b7d55a30b9b4  expr.c_qbe
4c72e8a009e3e6730969b7d55a30b9b4  fred.qbe
===
ea63b78d3fa59c8d540639d83df8cf75  gen.c_qbe
ea63b78d3fa59c8d540639d83df8cf75  fred.qbe
===
ccb643e15e8cc51969ee41aac2a691e7  misc.c_qbe
ccb643e15e8cc51969ee41aac2a691e7  fred.qbe
===
58fd815735a4ff3586a460884e58e700  opt.c_qbe
58fd815735a4ff3586a460884e58e700  fred.qbe
===
784b5c65469c655868761f5c4501739c  parse.c_qbe
784b5c65469c655868761f5c4501739c  fred.qbe
===
453d1ccb4e334b9e2852fac87a87bcff  scan.c_qbe
453d1ccb4e334b9e2852fac87a87bcff  fred.qbe
===
8d66ab6b83506b92ab7e747c9d645fa3  stmt.c_qbe
8d66ab6b83506b92ab7e747c9d645fa3  fred.qbe
===
d91591705e6e99733269781f4f44bf47  sym.c_qbe
d91591705e6e99733269781f4f44bf47  fred.qbe
===
30e39b30644aead98e47ccd5ebf6171c  targ6809.c_qbe
30e39b30644aead98e47ccd5ebf6171c  fred.qbe
===
0be7b46eb6add099be91cd3721ec4f09  targqbe.c_qbe
0be7b46eb6add099be91cd3721ec4f09  fred.qbe
===
73bd8faa39878c98f40397e0cf103408  tree.c_qbe
73bd8faa39878c98f40397e0cf103408  fred.qbe
===
7cf6e7e9ad7f587e31e3163aad1a40f3  tstring.c_qbe
7cf6e7e9ad7f587e31e3163aad1a40f3  fred.qbe
===
392531419455d54d333922f37570cb61  types.c_qbe
392531419455d54d333922f37570cb61  fred.qbe
===
d7a3ddeafccf98d03d2fe594e78f2689  wcc.c_qbe
d7a3ddeafccf98d03d2fe594e78f2689  fred.qbe
```

All the checksums are identical.

## Tue 04 Jun 2024 08:53:19 AEST

I rearranged the `Makefile` so that it is set up to run the triple test
with the QBE back-end. The level 0 binaries are built with `gcc`. The
level 1 binaries in `L1/` are built with the level 0 binaries. The
level 2 binaries in `L2/` will get built with the level 1 binaries.
Thus, the files in `L1/` and `L2` should be identical except for `wcc`
as BINDIR is different.

There's still a problem with `wcc`. I can do:

```
$ L1/wcc -S  wcc.c      and
$ L1/wcc -c  wcc.c
```

But when I try to do the link stage it just loops around doing:

```
$ L1/wcc -o wcc -v  wcc.c
Doing: cpp -nostdinc -isystem /usr/local/src/Cwj6809/include/qbe wcc.c 
  redirecting stdout to wcc.c_cpp
Doing: /usr/local/src/Cwj6809/L1/cscan 
  redirecting stdin from wcc.c_cpp
  redirecting stdout to wcc.c_tok
Doing: /usr/local/src/Cwj6809/L1/cparseqbe wcc.c_sym wcc.c_ast 
  redirecting stdin from wcc.c_tok
Doing: /usr/local/src/Cwj6809/L1/cgenqbe wcc.c_sym wcc.c_ast 
  redirecting stdout to wcc.c_qbe
Doing: qbe -o wcc.c_s wcc.c_qbe 
Doing: as -o wcc.c_o wcc.c_s 
Doing: cpp -nostdinc -isystem /usr/local/src/Cwj6809/include/qbe wcc.c 
  redirecting stdout to wcc.c_cpp
Doing: /usr/local/src/Cwj6809/L1/cscan 
  redirecting stdin from wcc.c_cpp
  redirecting stdout to wcc.c_tok
...
```

I have found the problem. Here's the test code:

```
#include <stdio.h>
int i;
int main() {
  for (i=1; i<= 10; i++) {
    if (i==4) { printf("I don't like 4 very much\n"); continue; }
    printf("i is %d\n", i);
  }
  return(0);
}
```

The `continue` should take us to the code that does the `i++`
before we do the `i<=10` test. But the compiler is taking
us to the `i<=10` test and we get stuck in an infinite loop
with `i` set to 4. Sigh.

I'm not sure how to deal with this. When I parse a `for` loop
I simply glue the postop tree to the end of the loop body
and treat it as a `while` loop. So there's only the label
before the condition test and the label at the loop's end.
There's no label between the body and the postop code.

For now I can rewrite `wcc` to avoid the problem. And, yes,
we now pass the triple test:

```
md5sum L1/* L2/* | sort
424d006522f88a6c8750888380c48dbe  L1/desym
424d006522f88a6c8750888380c48dbe  L2/desym
5da0fd17d14f35f19d1e1001c4ffa032  L2/wcc
6459d5698068115890478e4498bad693  L1/wcc
74ce22e789250c3c406980dab1c37df1  L1/detok
74ce22e789250c3c406980dab1c37df1  L2/detok
9cd8c07f0b66df2c775cfa348dfac4f7  L1/cscan
9cd8c07f0b66df2c775cfa348dfac4f7  L2/cscan
9fbe13d2b8120797ace55a37045f2a48  L1/cgenqbe
9fbe13d2b8120797ace55a37045f2a48  L2/cgenqbe
a9f109370f44ce15b9245d01b7b03597  L1/cparseqbe
a9f109370f44ce15b9245d01b7b03597  L2/cparseqbe
ebed2d69321e600bc3f5a634eb1ac1f8  L1/detree
ebed2d69321e600bc3f5a634eb1ac1f8  L2/detree
```

Yayy!!! I just merged the `bettercode` branch back in
to the `master` branch.

## Tue 04 Jun 2024 10:12:29 AEST

Back to the 6809 side now that we pass the triple test.
Current object sizes are:

```
   806 Jun  4 10:11 targ6809.o
   834 Jun  4 10:11 misc.o
  1062 Jun  4 10:11 targqbe.o
  2012 Jun  4 10:11 tstring.o
  2112 Jun  4 10:11 opt.o
  2658 Jun  4 10:11 detok.o
  2733 Jun  4 10:11 detree.o
  4399 Jun  4 10:11 types.o
  5513 Jun  4 10:11 ctreeopt.o
  5759 Jun  4 10:11 tree.o
  5858 Jun  4 10:11 parse.o
  7373 Jun  4 10:11 desym.o
  7638 Jun  4 10:11 stmt.o
  7868 Jun  4 10:11 cgen.o
  8801 Jun  4 10:11 sym.o
 11590 Jun  4 10:11 gen.o
 13256 Jun  4 10:11 wcc.o
 13942 Jun  4 10:11 scan.o
 17670 Jun  4 10:11 expr.o
 19156 Jun  4 10:11 decl.o
 21114 Jun  4 10:11 cgqbe.o
 34268 Jun  4 10:11 cg6809.o
```

## Tue 04 Jun 2024 11:38:09 AEST

I brought in some of the SubC tree optimisations.
Some didn't work and are commented out. The overall
code reduction is minimal.

Damn. I didn't check to later but they broke some of
the 6809 tests. Sigh.

## Tue 04 Jun 2024 11:44:46 AEST

I'm thinking of this idea, see this existing code:

```
L2:
        ldd -2,u   <-- d_holds "-2,u"
        cmpd #8
        bge L3
;		   <- now NOREG due to free all locns
        ldd -2,u
```

But surely we could keep this one because then we
wouldn't have to reload D? We would have to flush all
locations on a jump. I might see if I can do this.

Urgh, the tests pass but the code size is worse!!
I did add a simple peephole optimisation to avoid an
`ldd` after `std` to the same location.

## Tue 04 Jun 2024 13:25:58 AEST

Just did another fcc vs. wcc size comparison:

```
                 fcc     wcc
cg6809.o        28682   33364   1.16
cgen.o          5587    7742    1.39
cgqbe.o         16867   20999   1.24
ctreeopt.o      3965    5504    1.39
decl.o          14499   19058   1.31
desym.o         5881    7340    1.25
detok.o         1894    2609    1.38
detree.o        1848    2700    1.46
expr.o          11646   17527   1.50
gen.o           8745    11482   1.31
misc.o          742     834     1.12
opt.o           1519    2051    1.35
parse.o         4265    5797    1.36
scan.o          11312   13822   1.22
stmt.o          5649    7596    1.34
sym.o           6732    8781    1.30
targ6809.o      761     806     1.06
targqbe.o       903     1046    1.16
tree.o          5152    5745    1.12
tstring.o       2012    2012    1.00
types.o         2516    4397    1.75
wcc.o           10282   12897   1.25
                        Average 1.29
```

So 29% bigger than `fcc` at present.

I'm just looking at `types.c`. The first function is:

```
int inttype(int type) {
  return (((type & 0xf) == 0) && (type >= P_CHAR && type <= P_LONG));
}
```

with the AST being:

```
FUNCTION inttype
  RETURN
    LOGAND
      EQ
        AND
          IDENT rval type
          INTLIT 15
        INTLIT 0
      LOGAND
        GE
          IDENT rval type
          INTLIT 32
        LE
          IDENT rval type
          INTLIT 64
```

`wcc` generates awful code! But this is wrong:

```
        ldd 4,u
        anda #15	<- Should be #00
        andb #15
```

It's because `printlocation()` can't tell with literals
if we are doing an 8-bit, 16-bit or 32-bit operation.
Maybe I need to change one of the arguments to indicate
if I'm using A, B, D or Y? Done and it seems to work.

## Thu 06 Jun 2024 08:20:21 AEST

I've been struggling with producing better LOGAND and LOGOR
code, because the current code makes a lot of #0 and #1s
and then tests them. We should just be able to jump to
labels like `cgcompare_and_jump()`. I split them into two
functions, but I think I can merge them. Viz:

```
                 x || y                   x && y
--------------------------------------------------------------
        if (lhs true)  goto Ltrue   if (lhs false) goto Lfalse
        if (rhs false) goto Lfalse  if (rhs false) goto Lfalse
Ltrue:  ldd #1; goto Lend
Lfalse: ldd #0
Lend:
```

Really only the first line is different. I just need to pass
a different AST op down on the first line.

## Thu 06 Jun 2024 11:33:16 AEST

After much frustration I think I've finally fixed it. Yes, all the
tests still pass as does the triple test.

## Fri 07 Jun 2024 08:40:20 AEST

I realised that I could optimise:

```
#11
        ldx %1,u
        ldd 0,x
with
        ldd [%1,u]
```

and ditto D stores, B loads and stores. I finally realised that
the peephole optimiser rules are whitespace sensitive. I now have
a test input file I can run to check if it's working.

I've realised that I can move some of the code generation into
the peephole rules file. This will lessen the amount of C code
in the code generator. Example, I could generate `a >> 24` as

```
	ldd _a
	; cgshr 24
```

and the optimiser can replace it with a few Y/D/A/B exchanges and clears!

## Fri 07 Jun 2024 08:53:54 AEST

I wrote a Perl script to compare the `.o` sizes against `fcc`:

```
    cg6809.o:	1.07
      cgen.o:	1.30  *
     cgqbe.o:	1.15
  ctreeopt.o:	1.33
      decl.o:	1.05
     desym.o:	1.25
     detok.o:	1.38
    detree.o:	1.46
      expr.o:	1.21  *
       gen.o:	1.34  *
      misc.o:	1.12
       opt.o:	1.21
     parse.o:	1.33  *
      scan.o:	1.13
      stmt.o:	1.22  *
       sym.o:	1.17
  targ6809.o:	1.06
   targqbe.o:	1.16
      tree.o:	1.12
   tstring.o:	1.00
     types.o:	0.99
       wcc.o:	1.25
     Average:	1.19
```

I've starred the ones that are important.

## Fri 07 Jun 2024 09:53:22 AEST

I've been writing a few more peephole rules. But now I've
seen this code:

```
char *fred() { return(NULL); }

int main() {
  if (fred() != NULL) printf("fred was not NULL\n");
  return(0);
}
---
FUNCTION main
  IF
    NE
      FUNCCALL fred
      CAST 17
        INTLIT 0
---
        lbsr _fred	; Call fred
        std R0+0	; Store result
        ldd #0
        std R1+0	; Store 0 in temp
        ldd R0+0	; Now do the compare?!
        cmpd R1+0
        beq L3
```

Why can't we just `cmpd #0, beq L3`? Yes that works.
Now have to work out why we are generating this code!

Ah it's because of the cast. I added code to `cgwiden()`:
if the primary sizes are the same, do nothing.

This has brought the `wcc/fcc` ratios down:

```
      cgen.o:	1.30  now 1.23
      expr.o:	1.21  now 1.12
       gen.o:	1.34  now 1.30
     parse.o:	1.33  now 1.28
      stmt.o:	1.22  now 1.10
     Average:	1.19  now 1.15
```

## Fri 07 Jun 2024 16:40:13 AEST

I've written `objcompare` to compare function sizes. The worst
between `fcc` and `wcc` are:

```
enumerateAST:
   F/parse.o     85
   W/parse.o    151 1.78 (ratio)

genlabel:
     F/gen.o     15
     W/gen.o     26 1.73

mkastnode:
    F/tree.o    133
    W/tree.o    215 1.62
```

so I guess I check those out first.

## Sat 08 Jun 2024 10:02:01 AEST

I got an e-mail from Alan Cox about his recent inprovements with `fcc`.
I told him about this project and offered to send him the code.

While up at the arena I looked through all of `cg6809.c`. There's an
awful lot of:

```
  int primtype= cgprimtype(type);
  ...
  switch(primtype) {
    case P_CHAR:
    ...
    case P_INT:
    ...
    case P_POINTER:
    ...
    case P_LONG:
    ...
  }

```

We could get `gen.c` to send in the `primtype`, but I'm actually thinking
of getting in a character that represents the primary type. Then we could do:

```
int cgadd(int l1, int l2, char primchar) {

  load_d(l1);
  // Print out a pseudo-op
  fprintf(Outfile, "\tadd%c ", primchar); printlocation(l2, 0, primchar);
  cgfreelocn(l2);
  Locn[l1].type= L_DREG;
  d_holds= l1;
  return(l1);
}
```

Then the peephole optimiser can expand the pseudo-op with the actual code.
This would lose a heap of code from the code generator! There would be
an issue with INTLITS. If we generated:

```
    ; Pseudo-op
    addl #1234567
```

the peephole optimiser would have to do the `>>8` etc. Could be a pain.
Ah, a possible solution. For byte and int literals I can just print then,
e.g. `#2345`. For long literals I can print the top byte in decimal,
the second byte in decimal and the bottom 16 bits in decimal, e.g.

```
    ; Pseudo-op
    addl #0 #18 #54919
```

Then the optimiser can match against them. Would mean a lot of rules I guess.

## Sat 08 Jun 2024 10:11:45 AEST

I've written some other notes. Time to bring them in.

## Fix For and Continue

Currently we do

```
for (PRE; COND; POST)
  BODY
```

and convert to 

```
  PRE
Lstart:
  COND  (which could jump to Lend)
  BODY
  POST
Lend:
```

and the AST tree:

```
  GLUE
  / \
PRE WHILE
     /  \
   COND  \
        GLUE
        /  \
     BODY POST
```

But we don't use the middle child in the WHILE node. So we could build this
tree:

```
  GLUE
  / \
PRE WHILE
   /  |  \
COND  | BODY
    POST
```

In `genWHILE()` we can check for the presence of the middle child. If it's
there, we produce code:

```
  PRE	(done elsewhere)
  jump Lcond
Lstart:
  POST
Lcond:
  COND  (which could jump to Lend)
  BODY
Lend:
```

Now, any `break` will go to `Lend` and any `continue` will go to `Lstart`
and still do the post operation.

## Other Ideas

Would it be useful to have an `x_holds` tracker? It wouldn't add much
`cg6809.c` code as we don't use it but it might help shave a few percent
off the code output.

We need `cgshrconst()`. 

For shifts, we should check if the right is an INTLIT
and use `cgshlconst()` and `cgshrconst()` for certain
values.

For values 8, 16 and 24, just write a comment in the
assembly code: `leftshift_8` for example. Use the
peephole optimiser to put the code in. This means
we move lines of code out of the generator and into
the peephole optimiser. We could even do 1 and 2!

For the 8, 16, 24, we can do a bunch of register
exchanges and clearing to get the work done.

## Sun 09 Jun 2024 07:44:45 AEST

Alan wrote back suggesting it would be better to add macros to
the assembler than to push pseudo-op expansion on the peephole
optimiser. I guess that's true, especially as the optimiser loops
back to see if it can apply more optimisations. It would be slow.
Alan did mention tables, and I was wondering if I could use these.

For each "operation" there would be several size rules. Each rule
would have a string and an offset. If the offset is UNUSED, just
print the string, else print the string and the location with the
offset.

Do I just have a big table and a second table which holds the first
entry to use and the number of lines?

Anyway, before we even go there I still need to improve the code
density and reduce the wcc/fcc size ratio.

## Sun 09 Jun 2024 10:37:21 AEST

Some more peephole rules. We now have:


```
      cgen.o:	1.30  now 1.21
      expr.o:	1.21  now 1.10
       gen.o:	1.34  now 1.14
     parse.o:	1.33  now 1.27
      stmt.o:	1.22  now 1.06
     Average:	1.19  now 1.12
```

## Sun 09 Jun 2024 14:02:30 AEST

Musing on the way to/from Bunnings, I thought about the temps being
on the stack. I just checked, they are always allocated in incrementing
order and then either completely free or the last one is freed then
all are freed. This means I can change the allocation algorithm to
just increment a number.

When they go on the stack I'll need to adjust the `localOffset` to
account for the temps. However, I won't know what the final value will
be until the end of the function.

I was thinking, when doing `cgfuncpreamble(), I make the output file
read/write. At the point where I'm going to emit the `leas` to adjust
the stack, I'll save the offset using `ftell() and emit:

```
fprintf(Outfile, ";\tleas XXXXX,s\n");
```

Later on, if the `localOffset` is not zero, in `cgfuncpostamble()`
I can `fseek()` back to this point and overwrite the line with the
actual value. Slightly ugly but should work.

For now I'm going to change the temp allocation to just an increment.
It works; the new code is:

```
static int next_free_temp;

static int cgalloctemp() {
  return(next_free_temp++);
}
```

Later on I'll keep a `highest_free_temp`. If we allocate past it,
we can bump `localOffset` up by four.

We definitely need to replace `u` as the frame pointer because this
is adding a lot of extra code for really simple functions. It's why
`fcc` is doing better with `parse.c`.

Idea: we keep a `sp_adjust` variable, initally zero. Each time we
`pshs` it goes up by the appropriate size. Down for `puls` or `leas`.
As with `fcc`, to begin with we can check that it is zero when we
get to the function postamble.

On Tuesday I'll do the `sp_adjust` and lose the `u` frame pointer
first. Once that's working I can move the temporaries on to the stack.

## Sun 09 Jun 2024 14:48:20 AEST

I decided to add the `sp_adjust` but keep the the `u` frame pointer
as an intial step. I checked and `sp_adjust` is always zero in the
postamble for all the code I can throw at it.

## Sun 09 Jun 2024 14:57:49 AEST

Damnit, I started on the code to lose the `u` frame pointer.
Up to input026.c: OK, test 27 failed.

## Sun 09 Jun 2024 15:30:10 AEST

And ... done! It was actually easier than I expected. Just a few
dumb things I should have changed which got fixed. All tests OK.

We now have as a comparison:

```
      cgen.o:	1.18 size   6438
      expr.o:	1.04 size  12204
       gen.o:	1.06 size  10552
     parse.o:	1.23 size   5120
      stmt.o:	1.03 size   6047
     Average:	1.09
```

and the biggest `.o` files:

```
    cg6809.o:	1.03 size  29498
      decl.o:	0.97 size  13901
      expr.o:	1.04 size  12204
       gen.o:	1.06 size  10552
      scan.o:	1.09 size  12365
       wcc.o:	1.17 size  12055
```

`wcc` is fine but the code generator is `cg6809.o` and `gen.o` plus
a bunch of others, so that's going to be the pain point.

## Mon 10 Jun 2024 07:39:53 AEST

I decided to try out the `ftell()`, `fseek()` code to patch in the
stack change, just with the existing `leas` in the function preamble.
It works! Wow. Now I can get on with trying to put the temporaries on
the stack.

Ah, I'm stuck. Somehow we have to have some arrangement of:

```
  parameters
  return address
  locals
  temporaries
              <- sp
```

with known offsets for them all. But until I've allocated the most
temporaries consecutively will I know how much room they require.
That means I can't work out the offsets for the locals and parameters.
Even if I put the temporaries above the locals, I still can't work
out the parameter offsets.

Could I, when allocating a temporary, just add its size to `sp_adjust`
and return 0 as the temporary's offset? Effectively I've pushed the
temporary on the stack. And then lower `sp_adjust` when we free all
temporaries.

## Mon 10 Jun 2024 09:45:43 AEST

You know what. I just looked in `cg6809.c` and the spill code has been
commented out. So there's no need to spill temporaries. That means
there is no need to put them on the stack :-) Which means that I can
keep the current R0, R1 temporaries, yay! And I can lose the `fseek()`
code at the same time.

## Mon 10 Jun 2024 09:59:13 AEST

Let's now try to simplify the location allocation code in the same
way that I did the temp allocation code. No, because `gen.c` often
frees all but one register, so I need to keep track of that one. Damn.

I could copy the to-keep register down to R0, but it's a lot of effort.
I'd have to rewrite the `gen.c` code to receive the new register's location.
I'll park this for now.

## Mon 10 Jun 2024 10:24:44 AEST

Now going back to the table of output lines idea. I'll need to encode
offset and "register" for printlocation:

```
      1 printlocation(0, 'a');
      8 printlocation(0, 'b');
     10 printlocation(0, 'd');
      3 printlocation(0, 'e');
      3 printlocation(0, 'y');
      1 printlocation(1, 'b');
      3 printlocation(1, 'f');
      1 printlocation(2, 'a');
      6 printlocation(2, 'd');
      1 printlocation(2, 'y');
      1 printlocation(3, 'b');
```

and if we need to do a printlocation on a line. 

## Mon 10 Jun 2024 11:45:59 AEST

Taking a step back and looking at the function prototypes
in `cg6809.c`, the ones I think I can tabulate have
one or two register arguments, a type and a label to jump to.
Maybe I can alter `printlocation` to print nothing
when it's not needed, and to print out a label.

What I do need to do is to change the `type` that `gen.c`
sends down to the `cgXXX.c` functions so we don't have to
keep converting it. I've made a start but it dies after
some tests.

## Tue 11 Jun 2024 10:14:16 AEST

All 6809 tests now pass. I haven't rewritten the QBE backend yet.
By moving `cgprimtype()` up into `gen.c` I've saved 180 bytes
with the 6809 backend. Not much. It's probably not worth it,
especially if I have a table based approach where I can do the
`cgprimtype()` twice and then use the tables for many operations.
I'll park the changes in a side branch.

## Wed 12 Jun 2024 13:47:19 AEST

I added some `free()` code to `cgen.c` to clean up the AST trees.
Sometimes the left and right nodes are identical. Not sure why.
Anyway that works. I tried to free the local and parameter symbol
lists but now the QBE backend generates different code. So I've
commented this change out for now.

## Wed 12 Jun 2024 14:12:56 AEST

So I decided to go crazy and try this:

```
$ sh -x z
wcc -m6809 -o wcc wcc.c
wcc -m6809 -o cscan scan.c misc.c
wcc -m6809 -o detok detok.c tstring.c
wcc -m6809 -o detree detree.c misc.c
wcc -m6809 -o desym desym.c
wcc -m6809 -o cpeep cpeep.c
Incompatible types in binary expression on line 95 of cpeep.c
wcc -m6809 -o ctreeopt ctreeopt.c tree.c misc.c
wcc -m6809 -o cparse6809 decl.c expr.c misc.c opt.c parse.c stmt.c sym.c
              tree.c targ6809.c tstring.c types.c
wcc -m6809 -o cgen6809 cg6809.c cgen.c gen.c misc.c sym.c targ6809.c
              tree.c types.c
```

with the result:

```
-rwxr-xr-x 1 wkt wkt 12280 Jun 12 14:16 wcc
-rwxr-xr-x 1 wkt wkt 10583 Jun 12 14:16 cscan
-rwxr-xr-x 1 wkt wkt  7536 Jun 12 14:16 detok
-rwxr-xr-x 1 wkt wkt  8984 Jun 12 14:16 detree
-rwxr-xr-x 1 wkt wkt  8434 Jun 12 14:16 desym
-rwxr-xr-x 1 wkt wkt  7941 Jun 12 14:16 ctreeopt
-rwxr-xr-x 1 wkt wkt 27267 Jun 12 14:16 cparse6809
-rwxr-xr-x 1 wkt wkt 29615 Jun 12 14:16 cgen6809
```

Interesting.

Damn. `emu6809 ./cscan < detok.c_cpp > fred` produces
and empty output file.

## Wed 12 Jun 2024 15:34:38 AEST

I just had an idea. The problem could be my own assembly output,
or an interaction with libc, or the emulator not doing something
right. But we do have another 6809 compiler, `fcc`. So I could
build the binaries with `fcc` and see what happens.

Right, I now have binaries built with `fcc`. This time `cscan`
does produce output, but it goes into a loop around line 82
of `detok.c` using the input file `detok.c_cpp`. At least the
6809 `detok` binary works :-) Ah, `detok.c` only has 82 lines,
so somehow it's not detecting the end of file.

So perhaps my emulator isn't sending EOF correctly?

Also checking that `fcc` passes my tests. No, test67 fails but
the others are OK.

## Thu 13 Jun 2024 09:26:47 AEST

I've added some debug code to `scan.c` and it looks like I've found
an `fcc` bug:

```
  switch (c) {
    case EOF:
      t->token = T_EOF;
fprintf(stderr, "c is EOF, t->token is %d not %d\n", t->token, T_EOF);
      return (0);
```

produces the code:

```
        ldx #Sw67
        bra __switch
Sw67_1:
        clr [0,s]	; Supposedly zero t->token, but
        clr [1,s]	; [0,s] is different to [8,s] below
...
        clra
        clrb
        pshs d		; Push T_EOF which is zero
        ldd [8,s]
        pshs d		; Push t->token
        ldd #T176+0
        pshs d		; Push the string pointer
        ldd #_stderr+0
        pshs d		; Push the FILE *
        lbsr _fprintf+0
```

And, with my `wcc` compiling and building `cscan`, it looks like `fgetc()`
returns EOF on the first call. Very annoying.

## Thu 13 Jun 2024 09:56:36 AEST

I'm thinking of bring `fcc` and the bintools up to date with the Github
repo. Before I do that, here are the commits I am currently using:

 - bintools: bdb0076b5e3d4745aa08289d61e39f646d75805e
 - compiler: ffda85a94ce900423dc25a020fe62609ddcd46db

I've got the lastest of both with the compiler at commit
8a4b65b4d18be9528f3e5a6402b8e392e5ecc341. It runs the `wtests` OK
but it spitting out wrong code for some of the Fuzemsys libraries,
e.g.

```
$ /opt/fcc/bin/fcc -m6809 -S -O -D__m6809__ clock_gettime.c
$ vi +28 clock_gettime.s
        lbsr __shrul
        add 2,s
        adc 1,s
        adc ,s
```

which should be `addd, adcb, adca`. Now should I bother reporting
these to Alan? It means trying to find the commit that caused the
problem.

## Fri 14 Jun 2024 11:24:25 AEST

The `fgetc()` problem with `wcc` was because I'd defined `stdin` as
a pointer not an array of one `FILE` struct. `cscan` is now reading
characters but it fails elsewhere.

## Fri 14 Jun 2024 12:01:11 AEST

Argh! In the 6809 `cgswitch()` we are given the register (location)
that holds the switch value, but it was not being loaded into D.
A simple `load_d(reg)` fixed this. I added test 160.

## Fri 14 Jun 2024 12:17:20 AEST

We have progress. `cscan` and `detok` work and it looks like I'm making
a correct token stream file. The only issue is that it looks like the
Fuxiz `printf()` works differently than the Linux one, as I see these
sort of differences:

```
6464c6375
< 36: struct
---
> 27: struct
6469d6379
< 43: filename \"decl.c\"
6472c6382
< 36: void
---
> 1e: void
6476d6385
< 43: filename \"decl.c\"
6480c6389
< 36: struct
---
> 27: struct
```

The token numbers being printed are different but the Tstring used is
correct. And the double quotes in filenames are being quoted!

Actually that's not quite the truth. I had to make this change in `detok.c`:

```
<     *s++ = (char) ch;
---
>     *s = (char) ch; s++;
```

so I should investigate why as I can pass the QBE triple test with the
original line. OK that was an easy fix, thankfully.

## Fri 14 Jun 2024 12:42:40 AEST

Ah, I worked out why the token numbers were wrong. It seems that `cscan`
isn't detecting any keywords but converting everything to a STRLIT.
That's why I'm getting:

```
$ emu6809 cparse6809 decl.c_sym decl.c_ast < decl.c_tok 
unknown type:void on line 4 of /opt/wcc/include/6809/stdlib.h
```

as that's "void" not 30 (T_VOID).

## Fri 14 Jun 2024 15:19:25 AEST

It's because `if (!strcmp(...))` isn't working. Fixed.

## Sat 15 Jun 2024 09:32:15 AEST

Not fixed. It's because we are returning from a switch case. I have
a new test:

```
int keyword(char *s) {

  switch (*s) {
    case 'b': return(1);
    case 'c': if (!strcmp(s, "case")) return(2);
              return(3);
    case 'v': if (!strcmp(s, "void")) return(4);
              return(5);
    default: return(6);
  }
  return(0);
}
```

which works for QBE but always returns 6 with the 6809 backend.
Fixed now. The assembly code handling switch cases expects the
argument to be `int` but we were sending it a `char` with garbage
in the A register. I've changed `stmt.c` to widen a P_CHAR tree
to be P_INT if required.

It looks like the token streams are now good with T_VOID for "void" etc.

Moving on to the next phase:

```
$ emu6809 cparse6809 decl.c_sym decl.c_ast < decl.c_tok
unknown struct/union type:FILE on line 35 of /opt/wcc/include/6809/stdio.h
```

Looks like somehow the symbol table is getting mangled:

```
Searching for struct __stdio_file: missing!
Adding struct __stdio_file
Searching for struct __stdio_file: found it
Searching for struct __stdio_file: missing!
```

With more debug prints:

```
Searching for __stdio_file in list 778e class 0
Comparing against __stdio_file
  (and found it)
...
Searching for __stdio_file in list 778e class 0
  Did not find __stdio_file
```

where 778e is the head's value in hex and also
where the name pointer lives (it's the first member
of the struct). My debug code is:

```
fprintf(stderr, "Searching for %s in list %lx class %d\n",
					s, (long)list, class);
  for (; list != NULL; list = list->next) {
    if (list->name != NULL)
      fprintf(stderr, "Comparing against %s\n", list->name);
    if ((list->name != NULL) && !strcmp(s, list->name))
      if (class == 0 || class == list->class)
        return (list);
  }
fprintf(stderr, "  Did not find %s\n", s);
  return (NULL);
```

So does this mean that `list->name` is being set to NULL somehow?

## Sat 15 Jun 2024 11:14:39 AEST

Using the emulator and a write break, it looks like we are in
`scalar_declaration()` at the top, doing `*tree = NULL;`. Which
raises the question: are we not doing a double dereference, or how
are we getting a pointer into the struct table?

Even worse, that `tree` is a `struct ASTnode **` not even a symbol
table pointer! Argh!

## Sat 15 Jun 2024 11:42:43 AEST

I've got a `debug` file and I'm searching for 778E. I can see
a `newsym()` being made. I can see we go into `composite_declaration()`,
find the `rbrace()` and add a member to the struct.

## Sat 15 Jun 2024 12:24:59 AEST

Stepping back a bit, I can compile this program with the 6809-binary phases:

```
void printint(int x);
int main() { printint(5); return(0); }
```

so maybe I should just try compiling the test programs?

## Sat 15 Jun 2024 12:45:54 AEST

I have a test script now for this. Tests 1 and 2 are OK, 3 fails.

The 6809 `cparse6809` runs and creates outputs. So does the native
version of the compiler. The latter produces this symbol table:

```
int printf() id 1: global, 1 params, ctypeid 0, nelems 1 st_posn 0
    char *fmt id 2: param offset 0, size 2, ctypeid 0, nelems 1 st_posn 0
void main() id 3: global, 0 params, ctypeid 0, nelems 0 st_posn 0
int x id 4: local offset 0, size 2, ctypeid 0, nelems 1 st_posn 0
unknown type x id 0: unknown class, size 0, ctypeid 0, nelems 0 st_posn 0
```

The last line is the empty symbol to mark the end of one AST tree.
The tree looks like:

```
FUNCTION main
  ASSIGN
    INTLIT 1
    IDENT x
  FUNCCALL printf
    STRLIT rval "%d
"
    IDENT rval x
  ASSIGN
    ADD
...
```

Now, doing the same with the 6809 tools:

```
int printf() id 1: global, 1 params, ctypeid 0, nelems 1 st_posn 0
    char *fmt id 2: param offset 0, size 2, ctypeid 512, nelems 1 st_posn 0
void main() id 3: global, 0 params, ctypeid 0, nelems 0 st_posn 0
int x id 4: local offset 0, size 2, ctypeid 4, nelems 1 st_posn 0
unknown type  id 0: unknown class, size 0, ctypeid 0, nelems 0 st_posn 0
```

and

```
FUNCTION main
Unknown dumpAST operator:8745 on line 1 of
```

With some more debug code, the two `detree`s do:

```
      Native		    6809 binary
Next ASTnode op 32      Next ASTnode op 32
About to read in a name About to read in a name
We got main     	We got main
Next ASTnode op 29      Next ASTnode op 29
Next ASTnode op 29      Next ASTnode op 29
Next ASTnode op 29      Next ASTnode op 29
Next ASTnode op 29      Next ASTnode op 29
Next ASTnode op 29      Next ASTnode op 29
Next ASTnode op 29      Next ASTnode op 29
Next ASTnode op 29      Next ASTnode op 29
Next ASTnode op 29      Next ASTnode op 29
Next ASTnode op 29      Next ASTnode op -31959
```

Just checked, there are nine `op 29` nodes in sequence. Why
are we not reading this? Perhaps its a stdio problem. That's the
ninth 32-byte record we read in, and it seems we read in:

```
2B3E: D7 29 D2 28 D2 28 D2 29 D2 00 03 00 5D 00 00 00   .).(.(.)....]...
2B4E: 00 00 00 00 00 00 00 24 85 00 53 00 00 00 00 00   .......$..S.....
whereas before we were getting
2B62: 00 1D 00 00 00 00 00 00 74 A8 00 00 73 F4 00 09   ........t...s...
2B72: 00 0A 00 00 00 13 00 00 00 00 00 00 00 00 00 00   ................
```

and 0x001D is op 29.

## Sat 15 Jun 2024 14:09:01 AEST

OK it's an input issue. Here's the two records using `hd`:

```
00000100                    1d 00  00 00 00 00 00 74 a8 00
00000110  00 73 f4 00 09 00 0a 00  00 00 13 00 00 00 00 00
00000120  00 00 00 00 00 00 1d 00  00 00 00 00 00 6f 90 00
00000130  00 74 cc 00 0a 00 0b 00  00 00 0e 00 00 00 00 00
00000140  00 00 00 00 00 00
```
and a dumb dump of the file:

```
op 1d type 0 ctype 0 rvalue 0 left 74a8 mid 0 right 73f4 nodeid 9
leftid a midid 0 righid 13 sym 0 name NULL symid 0 size 0 linenm 0

op     5823 type   5322 ...
```

## Sun 16 Jun 2024 10:24:16 AEST

Back again today with a cold. I've changed the code to just dump
the buffer in hex. Using both `fcc` and `wcc` I get the same
behaviour of getting gibberish. Interestingly, if I remove the
code that calls my `fgetstr()` function, it works fine and doesn't
print any gibberish.

Hah. I rewrote the code from scratch and it still fails. Then I
decided to remove the `fgetc()` and replace with `fread()` and now
it works. So either `fgetc()` itself is bad or the code (compiled
by `fcc`) is bad. Or, it's an interaction between `fgetc()` and `fread()`.

## Sun 16 Jun 2024 10:59:40 AEST

SEE BELOW...

That helped a lot! I can now pass lots more tests. Tests 1 to 10 OK,
11 fails, 12 to 21 OK, 22 fails, 23 to 25 OK.

I guess I can compare the assembly files and see how they differ.
Actually I can also do the AST and symbol files. The AST files are
fine. But for the symbol files I see this:

```
$ diff sym 6sym
2c2
<     char *fmt id 2: param offset 0, size 2, ctypeid 0, nelems 1 st_posn 0
---
>     char *fmt id 2: param offset 0, size 2, ctypeid 512, nelems 1 st_posn 0
4,7c4,7
< int i id 4: local offset 0, size 2, ctypeid 0, nelems 1 st_posn 0
< char j id 5: local offset 0, size 1, ctypeid 0, nelems 1 st_posn 0
< long k id 6: local offset 0, size 4, ctypeid 0, nelems 1 st_posn 0
< unknown type k id 0: unknown class, size 0, ctypeid 0, nelems 0 st_posn 0
---
> int i id 4: local offset 0, size 2, ctypeid 4, nelems 1 st_posn 0
> char j id 5: local offset 0, size 1, ctypeid 5, nelems 1 st_posn 0
> long k id 6: local offset 0, size 4, ctypeid 6, nelems 1 st_posn 0
> unknown type  id 0: unknown class, size 0, ctypeid 0, nelems 0 st_posn 0
```

and the ctypeids are all wrong. Yes, they are in the actual sym file.
I added some debug code to `newsym()` to see what the `ctype` pointer
is set to. With the native compiler:

```
newsym printf ctype (nil) ctypeid 0
newsym fmt ctype (nil) ctypeid 0
newsym main ctype (nil) ctypeid 0
newsym i ctype (nil) ctypeid 0
newsym j ctype (nil) ctypeid 0
newsym k ctype (nil) ctypeid 0
```

and the 6809 version:

```
newsym printf ctype 00000 ctypeid 0
newsym fmt ctype 00003 ctypeid 512
newsym main ctype 00000 ctypeid 0
newsym i ctype 00002 ctypeid 4
newsym j ctype 00002 ctypeid 5
newsym k ctype 00002 ctypeid 6
```

and I doubt 2 and 3 are real pointers in memory!

## Sun 16 Jun 2024 11:52:03 AEST

Hmm. Some printfs later:

```
d_l B2 ctype 00000
s_d ctype 00002
newsym i ctype 00002 ctypeid 4
```

It looks like I'm not correctly passing the `ctype` to
`symbol_declaration()`. Yes, this doesn't look right:

```
        leax 6,s
        tfr x,d
        ldd [10,s]
        pshs d
        ldd 14,s
        pshs d
        pshs d		<-- Push twice with no D change?!
        ldd 8,s
        pshs d
        lbsr _symbol_declaration
        leas 8,s
```

and the C code is

```
sym = symbol_declaration(type, *ctype, class, &tree);
```

It seems I'm not loading the actual `*ctype` value before pushing it.
The AST tree has:

```
      FUNCCALL symbol_declaration
        IDENT rval type
        DEREF rval
          IDENT rval ctype
        IDENT rval class
        ADDR tree
```

and there's a bunch of other DEREF IDENTs in the tree elsewhere, but
this is the only one where we are not doing the deref.

No, it looks like we get into `cgcall()` with two locations marked
as D_REGS:

```
(gdb) p Locn[0]
$4 = {type = 7, name = 0x0, intval = 0, primtype = 3}   <===
(gdb) p Locn[1]
$5 = {type = 2, name = 0x0, intval = 12, primtype = 2}
(gdb) p Locn[2]
$6 = {type = 7, name = 0x0, intval = 0, primtype = 3}   <===
(gdb) p Locn[3]
$7 = {type = 2, name = 0x0, intval = 2, primtype = 2}
```

I think I can see the problem. In `gen_funccal()` we
generate the code to get all the argument values:

```
  for (i = 0, gluetree = n->left; gluetree != NULL; gluetree = gluetree->left) {
    // Calculate the expression's value
    arglist[i] =
      genAST(gluetree->right, NOLABEL, NOLABEL, NOLABEL, gluetree->op);
    typelist[i++] = gluetree->right->type;
  }
```

but several `cg6809` functions allocate a L_DREG location:
`cgwiden()`, `cgaddress()`, `cgderef()`. And on this line:

```
sym = symbol_declaration(type, *ctype, class, &tree);
```

I am getting the address of `tree` and dereferencing `ctype`!
Thus we have two locations which think they are L_DREG.

## Sun 16 Jun 2024 13:23:23 AEST

SEE ABOVE... That `fgets()` -> `fread()` change broke the tests so
I've reverted it for now and kept a copy of the new code in RCS.

I've added a `stash_d()` in `cgderef()` and now the assembly is:

```
        leax 6,s
        tfr x,d
        std R0+0	&tree into R0
        ldx 10,s
        ldd 0,x
        std R1+0	*ctype in R1
        ldd R0+0	&tree pushed
        pshs d
        ldd 14,s	class pushed
        pshs d
        ldd R1+0	*ctype pushed
        pshs d
        ldd 8,s		type pushed
        pshs d
        lbsr _symbol_declaration
which is optimised to
        leax 6,s
        stx R0+0	&tree into R0
        ldd [10,s]
        std R1+0	*ctype in R1
        ldd R0+0
        pshs d		&tree pushed
        ldd 14,s
        pshs d		class pushed
        ldd R1+0
        pshs d		*ctype pushed
        ldd 8,s
        pshs d		type pushed
        lbsr _symbol_declaration
```

## Sun 16 Jun 2024 13:54:28 AEST

That fixed the `ctype` bug. But it doesn't fix the `input011` problem.
At least now the trees and symbol tables are the same.

It looks like the compiler isn't emitting the right literal
sub-values for longs:

```
$ diff goodqbe badqbe 
49c49
< 	ldy #0
---
> 	ldy #30
150c150
< 	ldy #0
---
> 	ldy #1
158c158
< 	cmpy #0
---
> 	cmpy #5
189,190c189,190
< 	adcb #0
< 	adca #0
---
> 	adcb #1
> 	adca #1
```

In particular, we seem to be using the low half of the value
when we should be using the upper half.

Thats the const handling in `printlocation()` and it's the only
place where we do right shifts. I'll try and replace it with
some byte handling. Done (I'd written it on paper before) and
now we get up to test 31 FAIL. Actually that's error testing.
We are now up to test 58 fail :-)

## Sun 16 Jun 2024 15:09:58 AEST

The error is `Bad type in cgprimtype::0 on line 15 of main`.
The symbol table is fine but the tree shows INTLIT differences:

```
  good		   bad
FUNCTION main   FUNCTION main
  ASSIGN          ASSIGN
    INTLIT 12       INTLIT 12
    DEREF           DEREF
      ADD             ADD
        ADDR var2       ADDR var2
        INTLIT 0        INTLIT 48	<==
  FUNCCALL printf FUNCCALL printf
    STRLIT rval "%d"        STRLIT rval "%d"
    DEREF rval      DEREF rval
      ADD             ADD
        ADDR var2       ADDR var2
        INTLIT 0        INTLIT 48	<==
```

which are the member offsets in a struct.

Yes they are in the file, I added some printfs to detree:

```
INTLIT node value 12	INTLIT node value 12
INTLIT node value 0	INTLIT node value 48
INTLIT node value 0	INTLIT node value 48
INTLIT node value 99	INTLIT node value 99
INTLIT node value 2	INTLIT node value 48
INTLIT node value 2	INTLIT node value 48
INTLIT node value 4005	INTLIT node value 4005
INTLIT node value 3	INTLIT node value 48
INTLIT node value 3	INTLIT node value 48
INTLIT node value 0	INTLIT node value 48
INTLIT node value 2	INTLIT node value 48
INTLIT node value 3	INTLIT node value 48
INTLIT node value 0	INTLIT node value 48
INTLIT node value 2	INTLIT node value 48
INTLIT node value 3	INTLIT node value 48
INTLIT node value 0	INTLIT node value 0
```

I checked in `expr.c` where we build the INTLIT node:

```
right = mkastleaf(A_INTLIT, cgaddrint(), NULL, NULL, m->st_posn);
```

and both native and 6809 versions use the right position. So
how does it get corrupted to be 48? Ooh, I print the value
after the `mkastleaf()` and it's 48!!

`mkastleaf()` is getting the 48!

We have another double push:

```
        ldx 4,s
        ldd 20,x	<- but not storing m->st_posn
        lbsr _cgaddrint
        pshs d		<- push cgaddrint() result
        ldd #0
        pshs d		<- push NULL
        ldd #0
        pshs d		<- push NULL
        pshs d
        ldd #26
        pshs d		<- push A_INTLIT
        lbsr _mkastleaf
```

Question: why is the `cgaddrint()` result being pushed before
the NULLs? Ah, I'd missed a D stash in `cgcall()`. Yay, fixed.

## Mon 17 Jun 2024 07:49:35 AEST

This gets us up to input130 which fails!! This is doing

```
printf("Hello " "world" "\n");
```

The AST tree is:

```
FUNCTION 
  FUNCCALL printf
    STRLIT rval \"Hello \"
  RETURN
    INTLIT 0
```

which isn't right. The three strings are in the token stream.

Side node: `printf("My name is \"Warren\" you know!\n");`. `gcc`
gives `My name is "Warren" you know!`. But my compiler gives
`My name is \"Warren\" you know!`. That means my compiler is
interpreting the literal incorrectly, so I should fix it.

Fixed that, didn't fix test 130.

## Mon 17 Jun 2024 08:26:02 AEST

So the native `wcc` concatenates strings fine:

```
First strlit >foo< totalsize 3 >foo< litend 0x56267086dfe3
First strlit >Hello < totalsize 6 >Hello < litend 0x56267086e196
Next strlit >world< totalsize 11 >Hello world< litend 0x56267086e19b
Next strlit >
< totalsize 12 >Hello world
< litend 0x56267086e19c
```

but the 6809 one doesn't:

```
First strlit >Hello < totalsize 6 litval 08574 >Hello < litend 0857a
Next strlit >world< totalsize 11 litval 08564 >Hello < litend 0857f
Next strlit >
< totalsize 12 litval 08090 >Hello < litend 08580
```

OK, fixed. Now test 135 fails. Also, the only other test that fails is 162.

The symbol table and AST tree look fine.
It seems like the code generator is crashing. Looking at the debug trail,
we are in `cgcall()`. `ptrtype()` has returned 1 and `cgprimsize()` has
returned 2. We then do a `fprintf()`. So we must be doing this line:

```
  // Call the function, adjust the stack
  fprintf(Outfile, "\tlbsr _%s\n", sym->name);
```

Single-stepping the emulator, `sym->name` is fine, it points to "printf".
The string literal seems fine. And then we push $765E which the map file
says is Outfile. So it doesn't look like our code is wrong.

We fall into `vfnprintf()` and the switch statement for '%' with B holding
0x73 i.e. 's'. We get down to:

```
   __switchc+0005: CMPB ,X+         | -FHI-Z-- 00:73 7572 0001 0000 FC89
   __switchc+0007: BEQ $6410        | -FHI-Z-- 00:73 7572 0001 0000 FC89
   __switchc+000F: LDX ,X           | -FHI---- 00:73 0007 0001 0000 FC89
   __switchc+0011: JMP ,X           | -FHI---- 00:73 0007 0001 0000 FC89
0007: LSR <$00         | -FHI---C 00:73 0007 0001 0000 FC89
```

and that jump to $0007 is definitely wrong! Here's a dump of the switch
table:

```
7536: 00 14 00 55 5D 2D 55 6B 20 55 73 2B 55 73 23 55
7546: 7C 2A 55 84 2E 55 C2 6C 55 CA 68 55 D2 64 55 D9
7556: 69 55 D9 62 56 27 6F 56 2E 70 56 35 58 56 4E 78
7566: 56 55 75 56 5A 21 57 61 63 57 7F 73 00 07 58 06
7576: FD 96 00 05 FF FF 00 00 00 00 00 00 00 04 00 02
```

Let's rewrite that a bit:

```
00 14     $14 (20 entries)
00 555D   0: '\0'
2D 556B   1: '-'
20 5573   2: ' '
2B 5573   3: '+'
23 557C   4: '#'
2A 5584   5: '*'
2E 55C2   6: '.'
6C 55CA   7: 'l'
68 55D2   8: 'h'
64 55D9   9: 'd'
69 55D9  10: 'i'
62 5627  11: 'b'
6F 562E  12: 'o'
70 5635  13: 'p'
58 564E  14: 'X'
78 5655  15: 'x'
75 565A  16: 'u'
21 5761  17: '!'
63 577F  18: 'c'
73 0007  19: 's'  <== must have been tromped on
   5806  default
```

Yes, the jump should be to $57AC. Putting in some write breaks...
It looks like the offending code is at the end of `load_d()`:

```
     _load_d+$00C8: LBSR _mul
     _load_d+$00CB: ADDD #$757C     base of Locn[]
     _load_d+$00CE: TFR D,X
     _load_d+$00D0: LDD #$0007      
     _load_d+$00D3: STD ,X

which is   Locn[l].type= L_DREG;
```

The debug trace shows that `load_d()` is being called with -1 (NOREG)!
Leading up to this, we are doing a `switch` on 0x22 in `genAST()`
which is an A_RETURN.  `genAST()` loads NOREG and calls `cgreturn()`.

And in the native version:

```
Breakpoint 1, cgreturn (l=-1, sym=0x555555566200) at cg6809.c:1211
(gdb) s
load_d (l=-1) at cg6809.c:154
```

Hah! So `load_d()` is getting a NOREG! OK I added code in `cgreturn()`
to not load D when NOREG. Test 135 passes now. And so does test 162.
So all the tests now pass. Yay!!!

## Mon 17 Jun 2024 10:13:33 AEST

Now we are down to these issues when building the compiler with
itself:

- Incompatible types in binary expression on line 95 of cpeep.c
- Can't A_ASSIGN in genAST(), op:1 on line 180 of run_command

The first is pointer subtraction.
The second is, I think, because `stdin` on Fuzix is defined as
an array not a pointer, so we can't assign a pointer to an array.
Ah, `freopen()` already resets the incoming file handle, no need
to do the assign. That's fixed.

Now I can put in a band-aid fix for just char pointer subtraction.
I've done this, and I've added a test for it. All tests pass for:
triple test, QBE, 6809 and 6809 compiled compiler. Now we just need
to pass the triple test on the 6809 side!

## Mon 17 Jun 2024 14:47:53 AEST

So there is this line in `cpeep.c`:

```
next = &((*next)->o_next);
```

which the compiler doesn't like: & operator must be followed by an
identifier on line 155 of cpeep.c.

I guess I get to rewrite that line!

## Mon 17 Jun 2024 14:52:55 AEST

Damn. I'm going from the smallest file upwards with a script called
`smake`. I hit:

```
$ ./smake  types.c
Doing types.c
Unable to malloc in mkastnode() on line 103 of types.c
```

But the assembly files for these ones are OK:
tstring.c, targ6809.c, tree.c, detok.c, opt.c. The only minor
thing is:

```
$ diff detok.s S/detok.s 
536c536
< 	ldd #65535
---
> 	ldd #-1
```

which I think is OK. It is: I made the `.o` file from both versions
and they have the same checksum.

Damn. So now we need to get on to saving memory!

## Mon 17 Jun 2024 15:29:42 AEST

I had this idea. Can we serialise (and then free) subtrees
when we know that they are whole statements with no useful
rvalue?

I just wrote a `dumptree.c` which simply reads in and dumps
AST nodes. At line 103 of `types.c` we are up to node 231,
with the function `modify_type()` starting at node 76.
On 6809 AST nodes are size 32, so that's 4,992 bytes.

I also thought about how to `free()` AST nodes. I've tried
in a few places with no luck, e.g. in `serialiseAST()` and
in the `opt.c` functions. Sigh.

## Tue 18 Jun 2024 08:52:10 AEST

I added some `brk()/sbrk()` debug code in the 6809 emulator to
see how quickly the heap grows. Looking at who calls `malloc()`,
here is how many times it is called when compiling `types.c`
before we run out of heap:

```
    241 mkastnode
    460 newsym
      5 serialiseSymtab
    928 strdup
```

I added some `free()`s in `decl.c` to help with the `strdup()`s
in there. Just checked with the native compiler and, yes, there are
460 symbols in the symbol table. Yikes!

## Tue 18 Jun 2024 09:27:49 AEST

Hah! I didn't realise/remember that the compiler can have unions
in structs:

```
typedef union { int x; char c; } fred;
typedef struct { int foo; char *name; fred combo; } jim;
jim mynode;

int main() {
  mynode.foo= 12; mynode.name="Dennis"; mynode.combo.x=17;
  printf("%d %s %d\n", mynode.foo, mynode.name, mynode.combo.x);
  return(0);
}
```

Maybe I can put some data structures on a diet! I tried with the
symbol table with no success, sigh.

## Wed 19 Jun 2024 10:13:08 AEST

I've been trying to come up with a good idea for the symbol table.
I think I have one.

Firstly, when parsing we write out any non-local/param symbols to
the symbol table file as soon as possible and then free them.

We keep a single in-memory symbol table. The only things in this
table are symbols from the symbol table file that have been brought
in by a `findXXX()` operation.

In global context, this in-memory table will hold, e.g. typedefs
that are used by other things, e.g.

```
extern FILE *stdin;	// We need to know about FILE's size
```

In a function context, the table will hold all the symbols that the
function uses, including parameters and locals.

At the beginning of each function, we can free the list to get rid
of anything built-up during global parsing. At the end of each
function we can flush all the locals and parameters to the file and
then free the list.

This should keep the in-memory symbol table relatively small, but
help to minimise the number of times we go back to the file to get stuff.

I think we can keep most/all of the existing `sym.c` API, with a few
new functions to do the free/flush actions.

I've made a new git branch: symtable_revamp, for this. I've installed
the exsting compiler binaries in `/opt/wcc`. Because this is going to
break a heap of things, I'll make a new install area `/opt/nwcc`. That
way I can compare the old symbol table and tree vs. when the new code
is creating.

## Wed 19 Jun 2024 18:00:54 AEST

I've made a start at the new code. It isn't finished and it doesn't
compile yet. However, I feel that it's a good approach. I've got
the code to add symbols done, now I'm writing the code to find symbols.
It will be interesting to debug it and find out what mistakes I've
made and what situations I didn't forsee.

## Fri 21 Jun 2024 07:39:23 AEST

It's at the point where the code compiles and runs. It doesn't work yet,
of course. I've just got a known-good symbol table with:

```
$ wcc -m6809 -S -X gen.c
$ /opt/wcc/bin/desym gen.c_sym > goodsym
```

so when I do the same with the new code I can compare. I've added a slew
of `fprintf()`s to the code.

The code at present doesn't properly attach function params as members
of the function symbol. I can fix that. I was thinking, perhaps, of
adding the locals to the function symbol as well. There are places
in the gen code which walk both lists, so it would make sense to have
them available attached to the function symbol.

## Sat 22 Jun 2024 12:58:09 AEST

Some progress but I think I've hit another problem. We now have two
in-memory symbol lists, one for types (struct, union, enum, typedef)
and the other for variables and functions. Things that have members
(struct, union, enum, function) get their associated members (locals,
params for functions) added to a temp list and attached to the symbol.

Now the problem. In the original compiler I defined these lists of "classes":

```
// Storage classes
enum {
  C_GLOBAL = 1,                 // Globally visible symbol
  C_LOCAL,                      // Locally visible symbol
  C_PARAM,                      // Locally visible function parameter
  C_EXTERN,                     // External globally visible symbol
  C_STATIC,                     // Static symbol, visible in one file
  C_STRUCT,                     // A struct
  C_UNION,                      // A union
  C_MEMBER,                     // Member of a struct or union
  C_ENUMTYPE,                   // A named enumeration type
  C_ENUMVAL,                    // A named enumeration value
  C_TYPEDEF,                    // A named typedef
  C_STRLIT                      // Not a class: used to denote string literals
};
```

but this is conflating two ideas. For example, I can have a
`static struct foo { ... };`, so that the `foo` type is visible only
in this file.

What I need are two values in each symbol table. One holds 
that the symbol is a struct/union/enum/typedef (or not), and the
other holds the symbol's visibility (global, extern, static).

We already have structural types:

```
enum {
  S_VARIABLE, S_FUNCTION, S_ARRAY
};
```

so maybe I add the struct/union/enum/typedef to this. Also the strlit?
Then keep global/extern/static/local/member/param/enumval as the
storage classes? And rename it as visibility.

The last four will always be in the `member` list and their actual visibility
will be determined by their parent symbol.

Also note for enums: enum values don't belong to any specific enum type.
Also, enum type names don't really do anything, but we do have
to prevent redefinitions of them. So both can be stored in the main symbol
table: enum types as types and enum values as global symbols.

## Sun 23 Jun 2024 10:50:15 AEST

I've done the above and I'm slowly working my way there. Right now we
can read in all the global prototypes, enums, structs etc. fine. It
dies when we hit the first function with parameters. Hopefully not too
hard to fix.

## Mon 24 Jun 2024 09:41:38 AEST

Well ... I think there's a problem. Example:

```
int fred(int a, int b, int c);

int fred(int x, int y, int z) {
  return(x+y+x);
}
```

The prototype gets written to the symbol table file with `a,b,c`.
We now get the actual function with different parameter names.
We could remove the old member list and add the new parameters,
but these won't get written out to disk. Or, if we did write
this out to disk, then now we have two entries in the symbol table
for the same function. And we can't go and patch in the new variable
names as the new names might be bigger than the old ones. Damn!

I'm thinking of adding a function to invalidate an on-disk symbol.
It would use the existing `findSyminfile()` to re-find the symbol,
then write a -1 `id` to mark it invalid. Then we can write out the
new symbol. It's an ugly solution but I can't think of a better one
at the moment.

## Mon 24 Jun 2024 15:14:55 AEST

I've added the invalidate code. Seems to work. Right now the code
appears to write out symbols but not NULLing the Member pointers,
as I'm getting a function with a struct's members as variables :-)

## Tue 25 Jun 2024 09:12:08 AEST

I was loading symbols + their members from disk but I'd forgotten
to reset the Memb pointers to NULL once done. I've also noticed that
I'm writing symbols back out to disk multiple times. They get loaded
in as needed, then flushed (written) back at the end of a function. Damn!

We are now up to this line in `gen.c`:

```
type = n->left->type;
```

and the error message:

```
No member found in struct/union: :type on line 200 of gen.c
```

Up to here, the old/new parse trees are identical which makes me feel good!

## Tue 25 Jun 2024 09:35:21 AEST

I think I have to add some symbol searching by id. I've added some dump code
for the in-memory symbol tables. I see:

```
struct ASTnode: struct id 302: global, ctypeid 0, nelems 0 st_posn 0
    int op id 303: member, size 2, ctypeid 0, nelems 1 st_posn 0
    int type id 304: member, size 2, ctypeid 0, nelems 1 st_posn 2
!!! struct exit *ctype id 305: member, size 2, ctypeid 287, nelems 1 st_posn 4
    int rvalue id 306: member, size 2, ctypeid 0, nelems 1 st_posn 6
!!! struct nodeid *left id 307: member, size 2, ctypeid 302, nelems 1 st_posn 8
!!! struct nodeid *mid id 308: member, size 2, ctypeid 302, nelems 1 st_posn 10
!!! struct nodeid *right id 309: member, size 2, ctypeid 302, nelems 1 st_posn 1
```

The `ctype` pointers are pointing to the wrong types as indicated by the `!!!`.
When I load symbols in from disk, I'm loading the symbol and its members.
What I should do is, if the `ctype` is not NULL, find the matching `ctypeid`
symbol and link it back in. Damn!!!

## Tue 25 Jun 2024 11:27:27 AEST

I've added more code to load in symbols by id, and to link `ctype` fields
to the matching symbol. We can now get down to the last function in `gen.c`
and, up to that point, the trees are identical. I still haven't dealt with
the repeated same symbol writing to the symbol file yet.

OK found the problem with the last function in `gen.c`. I needed to NULL
the Functionid before looking for duplicate parameters. Without this,
we were checking against the parameters in the previous function!

We now generate the same AST tree as compared to the compiler before
the on-disk symbol table. Yay!

## Tue 25 Jun 2024 12:21:45 AEST

I fixed the duplicate on-disk symbols. As we are allocating ids
incrementally, simply record the highest one that we wrote out
and don't write ones at or below this id on the next round.

Now I'm trying to parse all the other C files and I'm having troubles.
Sigh. I've gone to the tests to see if I can parse them. Just a
couple are causing problems.

They were when a function call had no arguments, then function had
no parameters but it did have locals. That's because all the locals
are on the member list after the parameters. I had to fix up the
logic to stop when we hit the first local.

## Wed 26 Jun 2024 10:02:50 AEST

I've decided, now that we can parse all the tests, to try and get the
generator to run as well. I've got the generator code to compile,
and now comparing the assembly output from old/new compiler.

## Wed 26 Jun 2024 12:33:58 AEST

Ah, I wasn't generating the data for global variables. I've got the
code nearly working, but there are 10 tests still failing. It's sooo
frustrating!

## Wed 26 Jun 2024 14:29:00 AEST

Now I'm up to trying to deal with global string literals which appear
in the symbol table after the variable they get assigned to. Tricky.
I'll find a way I'm sure. Yes, output the strlits first then the
non-strlits. Sigh. All the 6809 tests now pass. Some of the QBE tests
fail, and they look like strlits. So I'll need to fix that up.

## Fri 28 Jun 2024 14:06:37 AEST

I needed to `strdup()` the string value in `cgqbe.c`. All the tests
now pass on QBE and 6809. But the current parser is still balking
at the compiler's own code.

So here is the code in `wcc.c` that we are failing on:

```
void addtmpname(char *name) {
  struct filelist *this;
  ...
}
```

In the parser, we have added the prototype for `addtmpname()` to the
in-memory symbol table (i.e. with the parameter) and we are in the
function's body. Now we are searching for the `struct filelist` type.
It's been flushed out of memory, so we need to load it back in.

In doing so, we have NULL'd `Membhead` and `Membtail`. Why? Because
we loaded the `struct filelist` in and then had to read its members.
But we needed the member list to append the local `this` variable
after the `name` parameter. Damn!!

## Sat 29 Jun 2024 08:15:25 AEST

I fixed the above by adding a private list just for `loadSym()` to
use when reading symbols in from the file. Now onto the next problem.

There's a line in `scan.c` which is `c = next();`. We seem to be finding
the `next` symbol as a variable. We are calling:

```
res= loadSym(sym, name, stype, id, 0, 1); // stype NOTATYPE, id 0
```

and getting back a member of a struct called `next`, not the function.
So there is some logic bug in `loadSym()` that needs fixing.

Yes. When we use `loadSym()` and there's no match, we skip the initlist.
But if it's a struct/union type, we don't skip the members. So, we leave
`loadSym()`, then come back to `loadSym()` and it then reads the members
in and compares against the name.

So, somehow we need to load the struct/union type and load but skip the
members. OK that wasn't too easy. If we are searching for a name, then
it's not a local, param or member. The first two are found by `findlocl()`
and the members are loaded recursively by `loadSym()` itself. I just
added another condition in the IF statement. We can now parse `scan.c`
but the generator dies.

Here are the current list of errors:

```
cg6809.c 6809:Duplicate local variable declaration:cmplist on line 1028 of cg6809.c
cgqbe.c: Can't find symbol with id:549 on line 71 of cgpreamble
cpeep.c: & operator must be followed by an identifier on line 155 of cpeep.c
decl.c:  Can't find symbol with id:503 on line 0 of (null)
desym.c: Unknown variable or function:dumpsym on line 221 of desym.c
expr.c:  Unknown variable or function:binexpr on line 156 of expr.c
gen.c:   Can't find symbol with id:511 on line 30 of Lfalse
scan.c:  Can't find symbol with id:350 on line 0 of (null)
stmt.c:  Unknown variable or function:if_statement on line 355 of stmt.c
sym.c:   Type mismatch: literal vs. variable on line 26 of sym.c
```

The first is only a 6809-side problem. The others (apart from the `cpeep.c` one)
seem to be a symbol which was invalidated but not replaced with a new definition.

## Tue 02 Jul 2024 10:47:44 AEST

Argh, getting frustrated. I think the parser is now working fine, now a
problem in the code generator. The AST tree stores the id of many symbols.
However, if we invalidate a function's symbol, we set the id to -1 and store
a new version of it in the symbol table, with a new id. But the AST might
still have the old id. Example:

```
cgqbe.c:static void cgmakeglobstrs();
cgqbe.c:  cgmakeglobstrs();
cgqbe.c:static void cgmakeglobstrs() { ... }
```

The first line will get a symbol put into the table. The second line
will use this id in the AST. The third line invalidates the symbol's
id and replaces it with a new one, but the AST remains the same.

I tried in `decl.c` to get the old id, invalidate that symbol,
make the new symbol and insert the old id. Fine, except that the
`sym.c` code remembers the highest id we wrote out before, and it
won't write out the new symbol because the id "has been already written".
Damn!

And all this because we need to make sure that the function's variable
names are stored and they could be different to the prototype ones.
Is there another way?

I think I'm going to enforce that the prototype parameter names
must match the actual function parameter names. That way I can
avoid invalidating anything.

## Tue 02 Jul 2024 11:28:58 AEST

Done and I've removed all the invalidation code. Apart from the
`cpeep.c` problem, all the other C files compile and we still
pass the tests.

Now I need to add `-D` pre-processor handling to `wcc.c`. Done.

## Tue 02 Jul 2024 11:47:40 AEST

Now a problem with `FILE *Symfile = NULL;` in `sym.c` which
doesn't seem to get added to the symbol table. Solved by
adding it to `data.h` like the other symbols that need to
be extern sometimes and sometimes not.

Now I'm trying the triple test. We get into an infinite loop
but only on some inputs:

```
$ L1/nwcc -S gen.c
$ L1/nwcc -S stmt.c
$ L1/nwcc -S cgqbe.c    <---
^C
$ L1/nwcc -S wcc.c      <---
^C
$ L1/nwcc -S cg6809.c   <---
^C
$ L1/nwcc -S expr.c
$ L1/nwcc -S decl.c
```

For `wcc.c` the symbol tree and AST files are fine, `nwcc` versus `L1/nwcc`.
So it must be the code generator getting stuck.

We seem to get into an infinite loop while `Searching for symbol id 143
in memory`.

## Tue 02 Jul 2024 13:11:14 AEST

No I think I've been bitten by this bug again. In `cgen.c`:

```
  // Now do the non string literals
  for (sym=Symhead; sym!=NULL; sym=sym->next) {
      if (sym->stype== S_STRLIT) continue;
```

Can we continue and still get the `sym=sym->next` to work? Nope.

I rewrote the code and, yay!!!! we now pass the triple test again. Phew!

## Tue 02 Jul 2024 13:21:31 AEST

I can build the 6809 compiler binaries again using `build6809bins`.
That means I'm now back to where I was at Mon 17 Jun 2024 trying to
`smake types.c`.

The scanner works but I'm getting:

```
emu6809 6/cparse6809 misc.c_sym misc.c_ast
New parameter name doesn't match prototype:stream on line 51
of /opt/wcc/include/6809/stdio.h
```

Damn! The executable sizes are:

```
6/cgen6809.map:   7BA9 B __end
6/cparse6809.map: 77D6 B __end
6/cscan.map:      2E70 B __end
6/ctreeopt.map:   2258 B __end
6/desym.map:      24AA B __end
6/detok.map:      218E B __end
6/detree.map:     26D2 B __end
6/nwcc.map:       3951 B __end
```

## Wed 03 Jul 2024 08:06:08 AEST

OK, it's the 6809 code which is wrong. In a new test, these lines:

```
  if (strcmp(c, c)) { printf("%s and %s are different\n", c, c); }
  if (!strcmp(c, c)) { printf("%s and %s are the same\n", c, c); }
```

produce:

```
Fisherman and Fisherman are different
Fisherman and Fisherman are the same
```

and the problem goes back to before the symbol table rewrite.

I think I can see the problem. The second one has this debug run:

```
     _strcmp+003E: LEAS 4,S         | -FHI---- 00:00 01E3 0001 1082 FDAC
     _strcmp+0040: PULS PC,U        | -FHI---- 00:00 01E3 0001 0000 FDB0
       _main+0032: LEAS 4,S         | -FHI---- 00:00 01E3 0001 0000 FDB4
       _main+0034: CMPD #$0000      | -FHI-Z-- 00:00 01E3 0001 0000 FDB4
       _main+0038: BNE $016A        | -FHI-Z-- 00:00 01E3 0001 0000 FDB4
       _main+003A: LDD #$0001       | -FHI---- 00:01 01E3 0001 0000 FDB4
       _main+003D: BRA $016D        | -FHI---- 00:01 01E3 0001 0000 FDB4
```

`strcmp()` returns. We compare against zero and load a one if it was zero
(the negation). However, for the first call:

```
     _strcmp+003E: LEAS 4,S         | -FHI---- 00:00 01E3 0000 1082 FDAC
     _strcmp+0040: PULS PC,U        | -FHI---- 00:00 01E3 0000 0000 FDB0
       _main+000D: LEAS 4,S         | -FHI---- 00:00 01E3 0000 0000 FDB4
       _main+000F: BEQ $0150        | -FHI---- 00:00 01E3 0000 0000 FDB4
       _main+0011: LDD $1105        | -FHI---- 10:78 01E3 0000 0000 FDB4
```

`strcmp()` returns zero but the Z flag isn't set. Thus the `BEQ` operation
isn't taken. So I need to do a `CMPD #$0000` before the `BEQ`, as per the
second call. OK fixed now.

## Wed 03 Jul 2024 08:43:20 AEST

Damn. We are running out of memory when compiling a small file like `misc.c`:

```
Doing misc.c
Unable to malloc member in loadSym() on line 27 of misc.c
cparse6809 failed
```

That's worse than before we revamped the symbol table. Why?!!

## Wed 03 Jul 2024 10:30:58 AEST

I've spent a bit of time with valgrind. I had forgotten some `free()`s
in `sym.c`. When I put them in, I failed the triple test. Argh! It
turned out that I was not initialising the `rvalue` field in the symbols
and this, not the `free()`s was the problem. I can now pass the triple
test again, and valgrind now shows that most of the mem leaks are with
the AST.

Back to using `smake` to get the 6809 compiler to compile itself.
Much slower now that we have them symbol table in a file.

This time we got up to `detree.c` before we ran out of memory.
That's better than before. But it now means that I need to try
and free the AST nodes. I tried this before and failed. Damn.

Ah. I'd added it to the code generator which worked/works, but not
the parser. Just added it now and still pass the triple test.

Wow. For the parser, valgrind tells me that the mem leak has fallen
from 100K to about 4K. Let's see if that helps with `smake`.

Well, we got a bit further. This time we fail in the generator with
`detree.c` but we can compile `desym.c`. We can parse `stmt.c` but
fail in the generator.

Interestingly, the assembly files that do get generated only differ
with this:

```
desym.s
582c582
<       ldd #65535
---
>       ldd #-1
```

which is good. So now I think I need to do a valgrind on the code generator.

Ah, I was loading the globals but not freeing the unused ones properly.
Fixed and still pass the triple test.

## Wed 03 Jul 2024 11:54:43 AEST

So with these improvements I can now `smake stmt.c` and `wcc.c`. After that:

```
Unable to malloc in mkastnode() on line 539 of scan.c, cparse6809 failed
Unable to malloc in mkastnode() on line 519 of gen.c, cparse6809 failed
Unknown variable or function:ftell on line 366 of sym.c, cparse6809 failed
Unknown AST operator:14336 on line -1279 of binexpr, cgen6809 failed
Unable to malloc in mkastnode() on line 86 of decl.c, cparse6809 failed
```

The `malloc()` ones are when we have really big functions, so the AST
tree is huge! And I guess there are a lot of symbols too. Do we try to
put the structs on a diet? I don't think I can easily serialise the
partial AST tree?

## Wed 03 Jul 2024 13:10:42 AEST

Struct dieting. symtable: some things could be chars not ints:
 - type, stype, class, st_hasaddr

We could also unionise ctype and ctypeid.

For the AST nodes:
 - these could be chars: type, rvalue
 - we could unionise the left/mid/right pointers and ids
 - we could unionise the sym pointer and the symid
 - do we need name if we can use sym->name?

## Wed 03 Jul 2024 14:48:23 AEST

Musing on partial AST trees ... In the generator we have `genfreeregs()`
which signals that there's no result needed. This gets called when
generating IF, WHILE, SWITCH, logical OR, logical AND and glue operations.
Maybe in the parser we could somehow serialise trees at these points.

We also have ids in each AST node. We could do something similar with
the current symbol table tree. We could have a `loadASTnode()` function.
Each time we need to go left/mid/right, we could `loadASTnode()` with
the id.

Maybe something like the following:

```
// Given an AST node id, load that AST node from the AST file.
// If nextfunc is set, find the next AST node which is a function.
// Allocate and return the node.
struct ASTnode *loadASTnode(int id, int nextfunc);
```

## Wed 03 Jul 2024 16:27:09 AEST

I'm now thinking that I can ditch the whole in-memory AST tree concept.
Instead, keep the tree on-disk as the AST file and left/mid/right ids,
which provide an implicit tree.

On the generator side, we simply use `loadASTnode()` each time we
need to traverse left/mid/right. On the parser side, we still make
nodes in memory, but once they are filled in (that means: left/mid/right,
hasaddr etc.) then we just write the node to disk and free it.

I think it also means that I can use the existing AST file and start work
on the generator side. Once that's working, I can go back and change the
parser side of things.

## Thu 04 Jul 2024 16:37:19 AEST

I've written but not tested the `loadASTnode()` code. I made a start
changing `gen.c` over to use it, but there are fiddly spots. I feel that
I need to step back a bit and do some thinking first. There is some list
walking code in `gen.c`; in a few functions we iterate the same list
multiple times. I don't really want to `loadASTnode()` the same nodes
multiple times in the same function.

I also need to think about the best places to `free()` AST nodes.
So I might put in some `free()`s first, get them working before I do
the `loadASTnode()` conversion.

## Thu 04 Jul 2024 17:36:11 AEST

I rewrote the `genAST()` code so that there was one return only at the end.
I added code in `tree.c` to free a single AST node. At the end of `genAST()`
I can free the left and middle AST node and pass the triple test. I can't
do that when I free the right-hand one. Need to investigate.

## Fri 05 Jul 2024 15:38:36 AEST

That is fixed. Somehow right can get set to left, so we must check it isn't
before freeing it.

I have spent some time setting new variables `nleft, nmid, nright` to
hold the sub-nodes. At present I'm just copying pointers but eventually
I will use `loadASTnode()`. In CASE and function calls we do have to
traverse the node linked list twice; can't get out of it. So I'll just
have to suck up the multiple disk reads for now. Maybe later build a
cache of some sort?

## Tue 09 Jul 2024 14:00:37 AEST

Back from the 3-day comedy cruise. I've rewritten `detree.c` to use
the `loadASTnode()` function. After fixing a few minor issues, it
prints out the same tree as the old version. That gives me confidence
that the `loadASTnode()` function works.

I just added `free()`s in `detree.c` and every single AST node and
associated name string gets free'd. That makes me very happy!!

## Tue 09 Jul 2024 15:38:05 AEST

I've added the `loadASTnode()` code to `gen.c` and it works up to the
first SWITCH test. That's pretty good :-) Once I get all the tests to
pass I will go back and work on freeing all the nodes that get allocated.

## Tue 09 Jul 2024 15:58:16 AEST

I had missed one left node pointer reference which needed to be loaded
with `loadASTnode()`. Now all the tests pass, yay!!!

## Wed 10 Jul 2024 07:39:53 AEST

I spent some time using valgrind last night and I've added a bunch
of `freeASTnode()` and friends to the code. We are down to about 3K
of un-free'd memory. That's excellent.

I think I'll stop on the code generation side. Now I need to work out,
on the parsing side, how to build AST nodes, write them to disk once
I have all the child node ids, and then free them.

The hard bit is to find one (or a few) places where I can do this. Right
now I'm doing it at the end of each function, but that requires too much
memory on the 6809 for large functions. 

## Wed 10 Jul 2024 10:33:05 AEST

I'm trying to move the enumeration of the AST nodes into `tree.c`, so
that a node gets an id when it's created. The problem is that I change
the tree structure in several places e.g. `optimise()` and I have to
find all the relinking and fix the child ids at the same time. Right
now for test 009 I have a set of AST nodes where a child node's id
is in several parent nodes! Not a tree!

Fixed. I wasn't zeroing the node ids when the child pointer was NULL.

## Thu 11 Jul 2024 14:13:17 AEST

With the new AST enumeration code, the tests pass for QBE and 6809.
Now trying to do the triple test and it fails. Comparing the `gcc` output
(Good) vs. the L1 output (Bad). The `cpp` file is fine. The symbol tables
are the same.

The AST files are the same except that the string literals seem different.
Looks like the newlines are getting corrupted, e.g.:

```
Good
----
00003d70  00 00 00 00 00 00 00 00  00 4f 75 74 20 6f 66 20  |.........Out of |
00003d80  73 70 61 63 65 20 69 6e  20 63 6d 64 61 72 67 73  |space in cmdargs|
00003d90  0a 00 23 00 00 00 10 00  00 00 00 00 00 00 00 00  |..#.............|

Bad
---
00003870  00 00 00 00 00 00 00 00  00 4f 75 74 20 6f 66 20  |.........Out of |
00003880  73 70 61 63 65 20 69 6e  20 63 6d 64 61 72 67 73  |space in cmdargs|
00003890  6e 00 23 00 00 00 10 00  00 00 00 00 00 00 00 00  |n.#.............|
```

Note the first byte on the last line: `0a` versus `6e`: `6e` is ASCII 'n'.
And the compiler before the symbol table fix has:

```
00003870  00 00 00 00 00 00 00 00  00 4f 75 74 20 6f 66 20  |.........Out of |
00003880  73 70 61 63 65 20 69 6e  20 63 6d 64 61 72 67 73  |space in cmdargs|
00003890  0a 00 23 00 00 00 10 00  00 00 00 00 00 00 00 00  |..#.............|
```
which is fine. And commit 6b870ee2d... Date:   Wed Jul 10 10:20:32 is also
fine. So at least I can narrow down the problem (I hope :-).

## Thu 11 Jul 2024 14:59:05 AEST

Looking at the diffs between the two sets of files, there is nothing that
sticks out as being a problem. Sigh.

## Thu 11 Jul 2024 15:10:40 AEST

It's weird. Here are some debug lines for reading/writing string literals
for both the `gcc` and `nwcc` passes in the triple test:

```
      1 Read in string >Out of space in cmdargs
<
      1 Dumping literal string/id >Out of space in cmdargs
<
```

This is the `gcc` parser. Newlines are OK. Now the code generator:

```
   1509 Read in string >Out of space in cmdargs
<
```
Again, fine. Now the `nwcc` compiled parser and generator:

```
      1 Read in string >Out of space in cmdargsn<
      1 Dumping literal string/id >Out of space in cmdargsn<
   1509 Read in string >Out of space in cmdargsn<
```

which is wrong.

## Thu 11 Jul 2024 15:34:54 AEST

Ah, the problem is back in the scanner! The Good scanner shows:

```
34: "Out of space in cmdargs
"
```

The Bad scanner shows:

```
34: "Out of space in cmdargsn"
```

## Fri 12 Jul 2024 07:41:16 AEST

I have been able to build a test case which abstracts some of the scanner
code and which exhibits the problem. Using the compiler from before the
on-disk symbol/AST changes, compared to the current one, I see this difference
in the generated AST tree:

```
28,30d27
<       CASE 110 ()
<         RETURN ()
<           INTLIT 10 ()
```

The case statement in the switch is completely missing!

```
    switch (c) {
      case 'n':
        return ('\n');
    }
```

Using my new `astdump` program, I can see that the right-hand AST child
id is missing in the SWITCH node in the file:

```
31 SWITCH type 0 rval 0
   line 14 symid 0 intval 1
   left 30 mid 0 right 0
```

where the good version has right 37. Ah, I found it. In `stmt.c` I link
the case sub-tree to the SWITCH node, but I'd forgotten to set up the
right-hand id. Fixed.

## Fri 12 Jul 2024 11:40:05 AEST

We are still not passing the triple test. In another directory I've
checked out the last commit which does pass the triple test:
50b82937c5da569b. I've just compiled (to asm) the compiler files
with both (Good/Bad), and now I can compare the intermediate file.

The token files are identical. Now looking at the `detree` outputs.
These are different: scan.c, cg6809.c, cgqbe.c.

Hmm, it looks like there are still missing case statements. Weird!
Found it, I was building a linked list of case trees and I had
forgotten to add in the child's nodeid along the way. We now pass
the triple test again!

Now I can get back to trying to serialise and free the AST nodes
in the parser.

## Fri 12 Jul 2024 12:12:08 AEST

I've made a start, but I'm not serialising some of the AST nodes
in input001.c. Damn! Ah, fixed. I can now pass all the tests except
for the one with the error "No return for function with non-void type".
I've had to comment out this test in the compiler, as I'm now freeing
the tree before I can check if there is a return!

Wow. We pass all the QBE tests, all the 6809 tests and the triple test!!!
I wonder if `valgrind` can tell me the maximum amount of memory allocated
at any point in time? Yes: https://valgrind.org/docs/manual/ms-manual.html

## Fri 12 Jul 2024 16:44:40 AEST

I just did a compile of the biggest compiler C file with the compiler.
Looking at the AST file with a script:

```
Seen 165 vs. unseen 2264
```

which means, when we read in an AST node that has children, it is more
likely that we haven't seen the children then we have.

And that means we should NOT seek back to the start of the file. Instead,
we should start from where we currently are, for the next search, and
exit if we hit the same seek point. 

I think. What I should do is add some counters in the `loadASTnode()`
function, change the search algorithm and see which is better.

## Sat 13 Jul 2024 09:56:58 AEST

OK, so I've built the 6809 compiler binaries with `build6809bins`. Now
I'm running `smake` on all the compiler C files to generate assembly
using the 6809 compiler binaries. It's as slow as hell! It's taken about
10 minutes and we are only up to line 145 in `cg6809.c`. I tried with
a couple of small files yesterday and the code generator died. This time
I'm going to try to compile everything, move the `*_*` files into a
subdir, then do the same with the native compiler. Then I can compare
the intermediate files to see if/where there are differences.

I'll definitely have to do an execution profile on the code generator
to see where the main bottlenecks are.

After about 30-40 minutes, we are still only halfway through the
code generation of `cg6809.c`! Compared to the native compiler, the
only difference I see so far is:

```
<       anda #0
<       andb #0
---
>       anda #255
>       andb #255
```

That's excellent as that is a very small difference and should not
be too hard to track down. We are off to see the kids for lunch, so
I can leave this running for several hours :-)

## Sat 13 Jul 2024 16:02:42 AEST

Back from the lunch. Damn. `smake` removes all the temp files at the
start, so everything is pretty much gone. But it also looked like all
the code generation phases failed. So I guess I can do them all again...
Looks like the parser ran out of memory on this one:
`Unable to malloc in mkastnode() on line 684 of gen.c` but none else so far.

## Sat 13 Jul 2024 16:51:56 AEST

Interesting. It looks like most of the code generation was successful, it's
just that the code generator doesn't stop nicely. Maybe I should `exit(0)`
at the end of `main()` instead of `return(0)`?

I'm also seeing a few:

```
<       ldd #65535
---
>       ldd #-1
```

which I think I've mentioned before.

So we have to check/fix these things:

 - 65535 / 1
 - and 0 / 255
 - segfault at end of code generation

Once they are fixed, we need to improve the speed of the code generation.
Right now I'm thinking of building a temp file which just consists of
a list of AST file offsets, one per node. To find AST node X, we multiply
by 4 (or shift left 2), `fseek()` to that point in the file and read in
the long offset at that point. Then we can `fseek()` into the AST file
to get the node. The biggest AST file has 4549 nodes, so that means the
file of offsets would be 18K long. Too big to keep as an in-memory array.

## Sun 14 Jul 2024 12:40:56 AEST

I just did a debug run of the 6809 `cgen` on `parse.c` as it's a decent
sized file. Yes, `main()` returns and we go off into never never land.
I'm also trying to see which function have the most instructions run.
The results are:

```
242540320 div16x16
87457045 _fread
34695064 _fgetstr
26953031 boolne
20521224 __mul
10534645 _loadASTnode
7960435 __minus
6814620 __divu
6150026 booleq
4794607 boolgt
4735523 boollt
4630436 __syscall
4594719 __not
3479025 boolult
2271540 _read
```

so, yes, I need to optimise `loadASTnode()`.

## Sun 14 Jul 2024 13:15:23 AEST

Looking at the reason for the crash, it seems like something is touching the
code in `crt0.s`. It should call `exit()` after `main()` returns, but I see:

```
       _main+0156: RTS              | -FHI-Z-- 00:00 8160 0000 0000 FD91
      __code+0028: NEG <$54         | -FHI-Z-- 00:00 8160 0000 0000 FD91
```

and `crt0.s` starts at $100. So I think I need to run the emulator with
a write break around $128.

Hmm, it seems to be one of the `fclose()` operations just before the
`return(0)` at the end of `main()`. Weird!

## Mon 15 Jul 2024 07:47:03 AEST

I just checked that `#65535` and `#-1` produce the same machine code,
so that's something I can ignore. For now, I'll remove the two `fclose()`s
and put in an `exit(0)` at the end of cgen.c. Done.

I will defer the `and #0` / `and #255` thing as the compiler is too slow!
I'll start writing the code to build the AST offset index file.

## Mon 15 Jul 2024 11:03:18 AEST

It's done, and we pass the normal tests but not the triple test. However
that's a good result for 45 minutes of work, and it seems to run faster.

## Mon 15 Jul 2024 12:09:33 AEST

I think maybe my compiler has a bug with `sizeof()`. Part of the index
building code is:

```
void mkASTidxfile(void) {
  struct ASTnode node;
  long offset, idxoff;

  while (1) {
    // Get the current offset
    offset = ftell(Infile);
    fprintf(stderr, "A offset %ld\n", offset);

    // Read in the next node, stop if none
    if (fread(&node, sizeof(struct ASTnode), 1, Infile)!=1) {
      break;
    }
    fprintf(stderr, "B offset %ld\n", offset);
    ...
```

and what I'm seeing when I run the L1 code generator:

```
A offset 133032
B offset -7491291578409943040
A offset 133120
B offset -7491311919375056895
A offset 133208
B offset 1
A offset 133308
B offset 0
```

I think I'm reading in more than I should be when I'm reading in the ASTnode.
No, actually it seems like it might be QBE. Looking at the ouput, we have:

```
C code
------
  struct ASTnode node;
  long offset, idxoff;

QBE code
--------
  %node =l alloc8 11	88 bytes in size
  %offset =l alloc8 1

ASM code
--------
subq $40, %rsp		??? should be 96 at least
movq %rdx, -8(%rbp)	-8 is the offset position on the stack
leaq -24(%rbp), %rdi	-24 is the node position on the stack
```

This seems to mean that `node` is only 16 bytes below `offset` on
the stack, even though it is 88 bytes in size. I've emailed the
QBE list for ideas.

Hmm. I made `node` a pointer and `malloc()`d it at the top of the
function, then `free()`d it at the end. Now I pass the triple test.
So yes it seems to be a QBE bug.

No, it's my bug. I got this reply on the QBE list:

> I believe alloc8 is alloc aligned on 8 byte boundary, not alloc 8*arg
> bytes. So alloc8 11 allocates 11, not 88 bytes, etc.

So I need to fix the QBE output to deal with this. TO FIX.

## Mon 15 Jul 2024 15:13:00 AEST

On the 6809 side, in `tree.c`, if I do this:

```
  n->leftid= 0;
  n->midid= 0;
  n->rightid= 0;
```

then we get:

```
        ldx 0,s
        ldd 12,s
        std 10,x
        ldx 0,s
        ldd 14,s
        std 12,x
        ldx 0,s
        ldd #0
        std 16,x
```

But if the code is this:

```
  n->leftid= n->midid= n->rightid= 0;
```

then we get:

```
        ldx 0,s
        ldd #0
        std 20,x
        std R0+0	<== Save the rvalue #0
        ldd 0,s
        addd #18
        tfr d,x
        std 0,x		<== Should reload R0
        ldd 0,s
        addd #16
        tfr d,x
        std 0,x		<== Should reload R0
```

which is incorrect. TO FIX!!!

Now I'm doing an `smake` on all the compiler files.
It's definitely a lot faster! No crashes yet ...
And it only too 16 minutes to compile all the files.
Now to compare them.

## Mon 15 Jul 2024 15:37:11 AEST

Wow. The only file that didn't compile was `gen.c`
as we ran out of memory in the parser: `Unable to
malloc in mkastnode() on line 684 of gen.c`.

The only assembly file differences are the `#-1 / #65535`
which is ignorable, and the `anda #0 / #255` problem
which I must fix.

So, if I can fix the latter issue and find a way to not
run out of memory in the parser with `gen.c`, then I
should be able to pass the triple test on the 6809 :-)

## Mon 15 Jul 2024 15:50:17 AEST

I think the `and` problem is a bug in `printlocation()`
with a int literal and 'e' or 'f' as the third argument.

## Tue 16 Jul 2024 09:21:25 AEST

I think I found the problem. We are doing `Locn[l].intval & 0xffff`
but `intval` is a long. That forces the compiler to widen the `0xffff`.
But, on the 6809, this is a negative value, so it gets widened to
`0xffffffff` not `0x0000ffff`. I'm trying a solution where I
cast/save to an `int` variable first before I do the AND.

Yes, that seems to work. I'm now running `smake` again, this time
I'm converting the assembly files to object files. Then I can
checksum them to see if they are identical.

The results are excellent with `gen.c` the only one that didn't
compile (parser runs out of memory):

```
143856ed08f470c9bc5f4b842dcc27bd  New/opt.o
143856ed08f470c9bc5f4b842dcc27bd  opt.o
18e04acd5b8e0302e95fc3c9cddcdac5  New/tree.o
18e04acd5b8e0302e95fc3c9cddcdac5  tree.o
1d42a151ccf415e102ece78257297cd9  New/tstring.o
1d42a151ccf415e102ece78257297cd9  tstring.o
43b84fc5d30ea22ceac9a21795518fc3  decl.o
43b84fc5d30ea22ceac9a21795518fc3  New/decl.o
45a18eb804fdc0c75f3207482ad8678a  detok.o
45a18eb804fdc0c75f3207482ad8678a  New/detok.o
57d10f0978232603854a6e18bf386cba  New/wcc.o
57d10f0978232603854a6e18bf386cba  wcc.o
76ed2bc3c553568d16880dfdd02053e7  New/parse.o
76ed2bc3c553568d16880dfdd02053e7  parse.o
88bfe3920d8f08d527447fef2c24dc3b  New/scan.o
88bfe3920d8f08d527447fef2c24dc3b  scan.o
8c24c919c06532977a68472c709c5e22  cg6809.o
8c24c919c06532977a68472c709c5e22  New/cg6809.o
8e4fd9f9e9923c20432ec7dae85965c5  expr.o
8e4fd9f9e9923c20432ec7dae85965c5  New/expr.o
8ed5104a4b18a1eb8dea33c5faf6c8bd  New/sym.o
8ed5104a4b18a1eb8dea33c5faf6c8bd  sym.o
9d11b1336597eaa6bcdac3ade6eb13ab  misc.o
9d11b1336597eaa6bcdac3ade6eb13ab  New/misc.o
a5fba53af6d4ca348554336db9455675  New/types.o
a5fba53af6d4ca348554336db9455675  types.o
beb8414b95be6f0de7494c21b16e1c53  New/stmt.o
beb8414b95be6f0de7494c21b16e1c53  stmt.o
c1e56e66055f7868ab20d13585a76eb0  cgen.o
c1e56e66055f7868ab20d13585a76eb0  New/cgen.o
ca8699919c901ed658c0ce5e0eb1d8e8  detree.o
ca8699919c901ed658c0ce5e0eb1d8e8  New/detree.o
d25a34d8dc2bb895b8c279d8946733c3  New/targ6809.o
d25a34d8dc2bb895b8c279d8946733c3  targ6809.o
deca10b552285f2de5c10e70547fd2a6  desym.o
deca10b552285f2de5c10e70547fd2a6  New/desym.o
```

So if I can write a suitable script, I should be able to pass the
triple test on the 6809 side :-)

## Tue 16 Jul 2024 10:47:01 AEST

I've rewritten `build6809bins` to make the 6809 binaries and
also make some front-end scripts which run the emulator on
the respective binary, so we have `native` executables. We
need this because the 6809 `wcc` will just run `cscan ...` not
`emu6809 cscan ...`.

But it's weird. `wcc` runs some of the phases but not all of them:

```
$ L1/wcc -m6809 -S -X -v targ6809.c 
Doing: cpp -nostdinc -isystem /usr/local/src/Cwj6809/include/6809 targ6809.c 
  redirecting stdout to targ6809.c_cpp
Doing: /usr/local/src/Cwj6809/L1/cscan 
  redirecting stdin from targ6809.c_cpp
  redirecting stdout to targ6809.c_tok
Doing: /usr/local/src/Cwj6809/L1/cparse6809 targ6809.c_sym targ6809.c_ast 
  redirecting stdin from targ6809.c_tok
```

and no code generation phase. I think for now I'll write a Perl version
of `wcc` so that I can get the triple test done.

## Tue 16 Jul 2024 11:35:05 AEST

I've broken one of the long SWITCH statements in `gen.c` into two;
hopefully this will allow the 6809 compiler to parse this without
running out of memory. I've checked and we pass all the tests.

`gen.c` now does compile using the L1 6809 compiler, and the resulting
object file is identical to that made by the native compiler. I've
had to put the SWITCH split in an `#ifdef` as the change stopped the
QBE triple test from passing. Weird!

## Tue 16 Jul 2024 14:09:06 AEST

I've written a Perl version of the `wcc` front-end, basically by
transliterating it. It now goes into the `L1` directory. I've just
modified `build6809bins` to build the `L2` binaries using the `L1`
6809 compiler binaries. So far the `L2` files that have been built
have the same checksum as the ones in `L1`, but it's still going ...

## Tue 16 Jul 2024 14:25:05 AEST

Oh, we came _sooo_ close!

```
$ md5sum L?/_* | sort 
0778e984e25d407d2067ac43d151d664  L2/_cgen6809  # Different
e47a9ab1ed9095f1c4784247c72cb1f8  L1/_cgen6809

0caee9118cb7745eaf40970677897dbf  L1/_detree
0caee9118cb7745eaf40970677897dbf  L2/_detree
2d333482ad8b4a886b5b78a4a49f3bb5  L1/_detok
2d333482ad8b4a886b5b78a4a49f3bb5  L2/_detok
d507bd89c0fc1439efe2dffc5d8edfe3  L1/_desym
d507bd89c0fc1439efe2dffc5d8edfe3  L2/_desym
e78da1f3003d87ca852f682adc4214e8  L1/_cscan
e78da1f3003d87ca852f682adc4214e8  L2/_cscan
e9c8b2c12ea5bd4f62091fafaae45971  L1/_cparse6809
e9c8b2c12ea5bd4f62091fafaae45971  L2/_cparse6809
```

and that's because, at the linker phase:

```
cgen.c_o: Unknown symbol '_genglobstr'.
cgen.c_o: Unknown symbol '_genglobsym'.
cgen.c_o: Unknown symbol '_genpreamble'.
cgen.c_o: Unknown symbol '_genAST'.
cgen.c_o: Unknown symbol '_genpostamble'.
gen.c_o: Unknown symbol '_genAST'.
```

Damn!!

I'll build the asm files for the C files that make the code generator,
using the native and the 6809 L1 compilers, and compare.

## Wed 17 Jul 2024 13:30:26 AEST

I've spent the last day writing the Readme.md for the next part of the
'acwj' journey. So far about 7,000 words and a couple thousand more to
go.

I think the 6809 `gen.c` problem was that I forgot to do `-DSPLITSWITCH`
when I compiled `gen.c`. Yes, now I have the same assembly except for
the -1 / 65535 change. Let's try the triple test again!

## Wed 17 Jul 2024 13:59:56 AEST

OK, I think I've passed the 6809 triple test:

```
$ md5sum L1/_* L2/_* | sort
01c5120e56cb299bf0063a07e38ec2b9  L1/_cgen6809
01c5120e56cb299bf0063a07e38ec2b9  L2/_cgen6809
0caee9118cb7745eaf40970677897dbf  L1/_detree
0caee9118cb7745eaf40970677897dbf  L2/_detree
2d333482ad8b4a886b5b78a4a49f3bb5  L1/_detok
2d333482ad8b4a886b5b78a4a49f3bb5  L2/_detok
d507bd89c0fc1439efe2dffc5d8edfe3  L1/_desym
d507bd89c0fc1439efe2dffc5d8edfe3  L2/_desym
e78da1f3003d87ca852f682adc4214e8  L1/_cscan
e78da1f3003d87ca852f682adc4214e8  L2/_cscan
e9c8b2c12ea5bd4f62091fafaae45971  L1/_cparse6809
e9c8b2c12ea5bd4f62091fafaae45971  L2/_cparse6809
```

All the binaries' checksums match!! I still don't have the 6809 `wcc`
binary working, so I'm relying on the Perl version. But I've been
able to compile the rest of the compiler's code with itself. Yayy!!!

## Thu 18 Jul 2024 09:52:53 AEST

I'm trying to work out why the 6809 `wcc` binary is failing. It runs
the C preprocessor (a native x64 binary) OK. Then it forks and runs
the 6809 `cscan` fine. Then it forks and runs the 6809 `cparse`.
This runs and completes; then `wcc` crashes with an unknown page-0 op.
Ah, I added an `exit(0)` before the final return which helps. Now
it crashes running the peephole optimiser.
