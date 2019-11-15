HSRCS= data.h decl.h defs.h
SRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	scan.c stmt.c sym.c tree.c types.c

ARMSRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	scan.c stmt.c sym.c tree.c types.c

cwj: $(SRCS) $(HSRCS)
	cc -o cwj -g -Wall $(SRCS)

cwjarm: $(ARMSRCS) $(HSRCS)
	cc -o cwjarm -g -Wall $(ARMSRCS)
	cp cwjarm cwj

clean:
	rm -f cwj cwjarm *.o *.s out

test: cwj tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

armtest: cwjarm tests/runtests
	(cd tests; chmod +x runtests; ./runtests)
