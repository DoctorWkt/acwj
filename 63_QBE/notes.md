## Mon 29 Nov 15:44:00 AEDT 2021

After passing all tests and trying the triple test, I have:

```
wkt@lenny:~/ownCloud/QBE$ ./cwj -S decl.c
decl.q:2354: invalid instruction type in alloc4
QBE translation of decl.q failed

wkt@lenny:~/ownCloud/QBE$ ./cwj -S gen.c 
Expected:; on line 479 of gen.c

wkt@lenny:~/ownCloud/QBE$ ./cwj -S scan.c 
scan.q:1885: label or } expected
QBE translation of scan.q failed

wkt@lenny:~/ownCloud/QBE$ ./cwj -S types.c 
types.q:496: label or } expected
QBE translation of types.q failed
```

# Mon 29 Nov 16:23:03 AEDT 2021

We have to copy parms that need addresses. We now have:

```
wkt@lenny:~/ownCloud/QBE$ ./cwj -S gen.c 
Expected:; on line 479 of gen.c

wkt@lenny:~/ownCloud/QBE$ ./cwj -S scan.c 
scan.q:1885: label or } expected
QBE translation of scan.q failed

wkt@lenny:~/ownCloud/QBE$ ./cwj -S types.c 
types.q:496: label or } expected
QBE translation of types.q failed
```

The `scan.c` and `types.c` issue is when we have:

```c
   if (something) {
     		// Do this
     break;	// out of a switch statement
   } else {
		// Other stuff
   }
```

The `break` generates a `jmp` instruction which is immediately followed by
the `jmp` to skip the `else` compound statement. QBE doesn't like two `jmps`
in a row. Not sure how to fix this.

The `gen.c` one is because I did a `return x` instead of `return(x)`
and the parser only accepts the latter.

Now I can compile each file, but `cwj` can't compile all files on
the one command line.

## Sun  5 Dec 11:33:02 AEDT 2021

Weird. `cwj` segfaults in `malloc()` for goodness sake! I'm now using dlmalloc.c
and I have this script to build `cwj2`:

```
#!/bin/sh
make cwj
./cwj -o cwj2 cg.c decl.c expr.c gen.c main.c misc.c opt.c scan.c stmt.c \
	sym.c tree.c types.c
```

When I run `cwj2` on any input, I get this:

```
$ ./cwj2 -S cg.c
Expected:comma on line 4 of /tmp/include/stdlib.h
```

In `declaration_list()`, these lines are failing:

```
    // We are at the end of the list, leave
    if (Token.token == et1 || Token.token == et2)
      return (type);
```

`et1` and `et2` are being passed in correctly as parameters, but at some
point `et1` is being tromped on, so the IF statement fails and we incorrectly
fall into the following `comma()` function call.

So I have two issues:

  1. Why is `malloc()` failing, and how is that even possible?
  2. What is causing `et1`'s value to be tromped on?

On a side note, when I compile `cwj` with `gcc -O2`, I get this size:

```
   text	   data	    bss	    dec	    hex	filename
  57028	   1884	    864	  59776	   e980	cwj
```

And when I use `cwj` with the new QBE back-end to build `cwj2`, I get:

```
   text	   data	    bss	    dec	    hex	filename
  38961	  12070	     48	  51079	   c787	cwj2
```

which seems to indicate that QBE is creating pretty tight assembly code.
Or, my back-end is still badly wrong!

## Sun  5 Dec 12:49:51 AEDT 2021

OK. I can't do "alloc4 2" for pointers as they need to be aligned on
8-byte boundaries. So, for locals on the stack, I now allocate in
multiples of 8 using "alloc8".

Now `cwj2` is in an infinite loop compiling `decl.c` and others. Looks
like the `class` local gets an "alloc8 1" an infinite number of times.

## Wed  8 Dec 13:20:21 AEDT 2021

I can now do `cwj -T -S cg.c` and `cwj2 -T -S cg.c`. There's no difference
in the AST tree dumps. But the QBE output differs:

```
  export function w $cgqbetype(w %type, ) {
  @L168
!   %.t1 =w loadsw $type	// Bad
--- 11,17 ----
!   %.t1 =w copy %type		// Good
```

## Wed  8 Dec 15:59:34 AEDT 2021

That was because the ternary code wasn't generating the conditional test
and jump, because it was calling logandor which wasn't doing it. The
code doing ==, != was, but the logandor wasn't. Fixed. Now `cg.q` is the
same between `cwj` and `cwj2`.

It now looks like, apart from the infinite loop problem, all the `.q`
files are identical between `cwj` and `cwj2`: `scan.q`, `misc.q`, `gen.q`,
`cg.q`, `opt.q`, `tree.q`, `types.q`, `sym.q`.

## Wed  8 Dec 19:08:34 AEDT 2021

Yay, I fixed the infinite loop. I had a `continue` statement in `cg.c`
which stopped the loop moving to the next element in the linked list.
I now have this script, `q2`, which performs the quadruple test:

```
#!/bin/sh
make clean; make install

for i in *.c; do ./cwj -S $i; done
cc -o cwj2 -g -no-pie *.s; rm -rf *.[qs]

for i in *.c; do ./cwj2 -S $i; done
cc -o cwj3 -g -no-pie *.s; rm -rf *.[qs]

for i in *.c; do ./cwj3 -S $i; done
cc -o cwj4 -g -no-pie *.s; rm -rf *.[qs]

md5sum cwj2 cwj3 cwj4; size cwj2 cwj3 cwj4

exit 0
```

I get these results at the end:

```
ab8fb443acca6ff658eb79dc331a22ca  cwj2
ce011a12c767c5783c8ec9c232f922ba  cwj3
b7f9d54c082fb43c91d83cbea01a7755  cwj4
   text	   data	    bss	    dec	    hex	filename
  44113	  12070	     48	  56231	   dba7	cwj2
  44113	  12070	     48	  56231	   dba7	cwj3
  44113	  12070	     48	  56231	   dba7	cwj4
```

So it looks like I am passing the quadruple test, but it's interesting
that the MD5 checksums on the files are different. Hmm, an `nm` on all
three executables produces exactly the same results.

## Wed  8 Dec 19:16:53 AEDT 2021

But I can't run the `make triple` in the `Makefile`, as I'm still getting
a segfault in `malloc()` from the line in `sym.c`:

```
struct symtable *node = (struct symtable *) malloc(sizeof(struct symtable));
```

Maybe it's my Devuan system? I'll try in `minnie`. Nope, same there. I'm
suspecting that I'm either running out of heap or stack. Given that I
never do any `free()` operations, it's likely that I should start adding
some to clean up after all my `malloc()`s.

Guess I should break out `valgrind` and use it to work out where to put
some `free()`s :-)

## Thu  9 Dec 11:20:14 AEDT 2021

Hah. I used `valgrind` and it found I was overwriting memory. This was
happening in `gen_funccall()` when I was creating the list of arguments
and their type. Some function calls have more arguments than the
expected number of parameters. Think `printf()` as an example. Now I
can run `make quad` from the Makefile and it all works fine.

I put in the `-O2` flag to build `cwj`, then did `make quad` and got these
sizes:

```
   text	   data	    bss	    dec	    hex	filename
  52820	   1884	    864	  55568	   d910	cwj
  44257	  12070	     48	  56375	   dc37	cwj2
  44257	  12070	     48	  56375	   dc37	cwj3
  44257	  12070	     48	  56375	   dc37	cwj4
```

So, in fact, QBE produces tighter machine code than `gcc`! I can understand
the difference in data size. That will be all the read-only strings which
are the same and which `gcc` keeps only one copy. Well, also, perhaps all
those strings are in the text section which could explain why the text size
is big.

Anyway, the total size between `gcc` and QBE is 55568 versus 56375, so I'd
say that's a big win. Before I rewrote the backend to use QBE, the original
code generator produced a `cwj2` with size 110602. Excellent. That's nearly
twice the new size.

In terms of SLOC, the new `cg.c` is 511 lines, the old one was 594 lines.
Overall, not much difference. I'm sure there would be some refactoring
that could be done now, but I'm not much interested in doing it.

I've just done an indent and some further small cleanup.

