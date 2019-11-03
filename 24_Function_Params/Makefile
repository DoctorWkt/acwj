HSRCS= data.h decl.h defs.h
SRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	scan.c stmt.c sym.c tree.c types.c

ARMSRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	scan.c stmt.c sym.c tree.c types.c

comp1: $(SRCS) $(HSRCS)
	cc -o comp1 -g -Wall $(SRCS)

comp1arm: $(ARMSRCS) $(HSRCS)
	cc -o comp1arm -g -Wall $(ARMSRCS)
	cp comp1arm comp1

clean:
	rm -f comp1 comp1arm *.o *.s out

test: comp1 tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

armtest: comp1arm tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

test27: comp1 input27a.c input27b.c
	./comp1 input27a.c
	cc -o out input27b.c out.s lib/printint.c 
	./out
