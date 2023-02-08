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

test8: comp1 tests/input08
	./comp1 tests/input08
	cc -o out out.s
	./out

testn: compn tests/runtestsn
	(cd tests; chmod +x runtestsn; ./runtestsn)

test8n: compn tests/input08
	./compn tests/input08
	nasm -f elf64 out.s
	cc -no-pie -o out out.o
	./out
