# Define the location of the include directory
# and the location to install the compiler binary
INCDIR=/tmp/include
BINDIR=/tmp

HSRCS= data.h decl.h defs.h
SRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	opt.c scan.c stmt.c sym.c tree.c types.c

ARMSRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	opt.c scan.c stmt.c sym.c tree.c types.c

cwj: $(SRCS) $(HSRCS)
	cc -o cwj -g -Wall -DINCDIR=\"$(INCDIR)\" $(SRCS)

cwjarm: $(ARMSRCS) $(HSRCS)
	cc -o cwjarm -g -Wall $(ARMSRCS)
	cp cwjarm cwj

install: cwj
	mkdir -p $(INCDIR)
	rsync -a include/. $(INCDIR)
	cp cwj $(BINDIR)
	chmod +x $(BINDIR)/cwj

clean:
	rm -f cwj cwjarm *.o *.s out a.out

test: install tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

armtest: cwjarm tests/runtests
	(cd tests; chmod +x runtests; ./runtests)
