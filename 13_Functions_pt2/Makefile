SRCS= cg.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c sym.c tree.c types.c
SRCN= cgn.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c sym.c tree.c types.c

comp1: $(SRCS)
	cc -o comp1 -g $(SRCS)

compn: $(SRCN)
	cc -o compn -g $(SRCN)

clean:
	rm -f comp1 compn *.o *.s out

test: comp1 tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

test14: comp1 tests/input14 lib/printint.c
	./comp1 tests/input14
	cc -o out out.s lib/printint.c
	./out

testn: compn tests/runtestsn
	(cd tests; chmod +x runtestsn; ./runtestsn)

test14n: compn tests/input14 lib/printint.c
	./compn tests/input14
	nasm -f elf64 out.s
	cc -no-pie -o out lib/printint.c out.o
	./out
