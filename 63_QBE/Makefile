# Define the location of the include directory
# and the location to install the compiler binary
INCDIR=/tmp/include
BINDIR=/tmp

HSRCS= data.h decl.h defs.h incdir.h
SRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	opt.c scan.c stmt.c sym.c tree.c types.c

cwj: $(SRCS) $(HSRCS)
	cc -o cwj -g -Wall $(SRCS)

incdir.h:
	echo "#define INCDIR \"$(INCDIR)\"" > incdir.h

install: cwj
	mkdir -p $(INCDIR)
	rsync -a include/. $(INCDIR)
	cp cwj $(BINDIR)
	chmod +x $(BINDIR)/cwj

clean:
	rm -f cwj cwj[0-9] *.o *.s *.q out a.out incdir.h

test: install tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

# Run the tests, stop on the first failure
stoptest: install tests/runtests
	(cd tests; chmod +x runtests; ./runtests stop)

# Run the tests with the
# compiler that compiled itself
test2: install tests/runtests2 cwj2
	(cd tests; chmod +x runtests2; ./runtests2)

# Try to do the triple test
triple: cwj3
	size cwj[23]

# Paranoid: quadruple test
quad: cwj4
	size cwj[234]

cwj4: cwj3 $(SRCS) $(HSRCS)
	./cwj3 -o cwj4 $(SRCS)

cwj3: cwj2 $(SRCS) $(HSRCS)
	./cwj2 -o cwj3 $(SRCS)

cwj2: install $(SRCS) $(HSRCS)
	./cwj  -o cwj2 $(SRCS)
