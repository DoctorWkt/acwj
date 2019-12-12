# Define the location of the include directory
# and the location to install the compiler binary
INCDIR=/tmp/include
BINDIR=/tmp

HSRCS= data.h decl.h defs.h incdir.h
SRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	opt.c scan.c stmt.c sym.c tree.c types.c

ARMSRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	opt.c scan.c stmt.c sym.c tree.c types.c

cwj: $(SRCS) $(HSRCS)
	cc -o cwj -g -Wall $(SRCS)

cwjarm: $(ARMSRCS) $(HSRCS)
	cc -o cwjarm -g -Wall $(ARMSRCS)
	cp cwjarm cwj

incdir.h:
	echo "#define INCDIR \"$(INCDIR)\"" > incdir.h

install: cwj
	mkdir -p $(INCDIR)
	rsync -a include/. $(INCDIR)
	cp cwj $(BINDIR)
	chmod +x $(BINDIR)/cwj

clean:
	rm -f cwj cwj[0-9] cwjarm *.o *.s out a.out incdir.h

test: install tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

# Run the tests with the
# compiler that compiled itself
test0: install tests/runtests0 cwj0
	(cd tests; chmod +x runtests0; ./runtests0)

armtest: cwjarm tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

# Try to do the triple test
triple: cwj1
	size cwj[01]

# Paranoid: quadruple test
quad: cwj2
	size cwj[012]

cwj2: cwj1 $(SRCS) $(HSRCS)
	./cwj1 -o cwj2 $(SRCS)

cwj1: cwj0 $(SRCS) $(HSRCS)
	./cwj0 -o cwj1 $(SRCS)

cwj0: install $(SRCS) $(HSRCS)
	./cwj  -o cwj0 $(SRCS)
