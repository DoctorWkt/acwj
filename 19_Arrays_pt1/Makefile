SRCS= cg.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c \
	sym.c tree.c types.c

SRCN= cgn.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c \
	sym.c tree.c types.c

ARMSRCS= cg_arm.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c \
	sym.c tree.c types.c

comp1: $(SRCS)
	cc -o comp1 -g -Wall $(SRCS)

compn: $(SRCN)
	cc -o compn -g -Wall $(SRCN)

comp1arm: $(ARMSRCS)
	cc -o comp1arm -g -Wall $(ARMSRCS)
	cp comp1arm comp1

clean:
	rm -f comp1 comp1arm compn *.o *.s out

test: comp1 tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

armtest: comp1arm tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

testn: compn tests/runtestsn
	(cd tests; chmod +x runtestsn; ./runtestsn)

test20: comp1 tests/input20.c lib/printint.c
	./comp1 tests/input20.c
	cc -o out out.s lib/printint.c
	./out

armtest20: comp1arm tests/input20.c lib/printint.c
	./comp1 tests/input20.c
	cc -o out out.s lib/printint.c
	./out

test20n: compn tests/input20.c lib/printint.c
	./compn tests/input20.c
	nasm -f elf64 out.s
	cc -no-pie -o out lib/printint.c out.o
	./out
