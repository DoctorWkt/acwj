SRCS= cg.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c sym.c tree.c
SRCN= cgn.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c sym.c tree.c

comp1: $(SRCS)
	cc -o comp1 -g $(SRCS)

compn: $(SRCN)
	cc -o compn -g $(SRCN)

clean:
	rm -f comp1 compn *.o *.s out

test: comp1 tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

test6: comp1 tests/input06
	./comp1 tests/input06
	cc -o out out.s
	./out

testn: compn tests/runtestsn
	(cd tests; chmod +x runtestsn; ./runtestsn)

test6n: compn tests/input06
	./compn tests/input06
	nasm -f elf64 out.s
	cc -no-pie -o out out.o
	./out
