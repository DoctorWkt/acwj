HSRCS= data.h decl.h defs.h
SRCS= cg.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c \
	sym.c tree.c types.c

SRCN= cgn.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c \
	sym.c tree.c types.c

ARMSRCS= cg_arm.c decl.c expr.c gen.c main.c misc.c scan.c stmt.c \
	sym.c tree.c types.c

comp1: $(SRCS) $(HSRCS)
	cc -o comp1 -g -Wall $(SRCS)

compn: $(SRCN) $(HSRCS)
	cc -o compn -g -Wall $(SRCN)

comp1arm: $(ARMSRCS) $(HSRCS)
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
