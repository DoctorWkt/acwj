SRCS= cg.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c \
	sym.c tree.c types.c

ARMSRCS= cg_arm.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c \
	sym.c tree.c types.c

comp1: $(SRCS)
	cc -o comp1 -g -Wall $(SRCS)

comp1arm: $(ARMSRCS)
	cc -o comp1arm -g -Wall $(ARMSRCS)
	cp comp1arm comp1

clean:
	rm -f comp1 comp1arm *.o *.s out

test: comp1 tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

armtest: comp1arm tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

test17: comp1 tests/input17.c lib/printint.c
	./comp1 tests/input17.c
	cc -o out out.s lib/printint.c
	./out

armtest17: comp1arm tests/input17.c lib/printint.c
	./comp1 tests/input17.c
	cc -o out out.s lib/printint.c
	./out
