# Define the location of the include directory
# and the location to install the compiler binary
INCDIR=/tmp/include
BINDIR=/tmp

HSRCS= data.h decl.h defs.h incdir.h
SRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	opt.c scan.c stmt.c sym.c tree.c types.c

SRCN= cgn.c decl.c expr.c gen.c main.c misc.c \
	opt.c scan.c stmt.c sym.c tree.c types.c

ARMSRCS= cg_arm.c decl.c expr.c gen.c main.c misc.c \
	opt.c scan.c stmt.c sym.c tree.c types.c

cwj: $(SRCS) $(HSRCS)
	echo "#define INCDIR \"$(INCDIR)\"" > incdir.h
	cc -o cwj -g -Wall -DINCDIR=\"$(INCDIR)\" $(SRCS)

compn: $(SRCN) $(HSRCS)
	echo "#define __NASM__ 1" >> incdir.h
	cc -D__NASM__ -o compn -g -Wall -DINCDIR=\"$(INCDIR)\" $(SRCN)

cwjarm: $(ARMSRCS) $(HSRCS)
	echo "#define INCDIR \"$(INCDIR)\"" > incdir.h
	cc -o cwjarm -g -Wall $(ARMSRCS)
	cp cwjarm cwj

incdir.h:
	echo "#define INCDIR \"$(INCDIR)\"" > incdir.h

install: cwj
	mkdir -p $(INCDIR)
	rsync -a include/. $(INCDIR)
	cp cwj $(BINDIR)
	chmod +x $(BINDIR)/cwj

installn: compn
	mkdir -p $(INCDIR)
	rsync -a include/. $(INCDIR)
	cp compn $(BINDIR)
	chmod +x $(BINDIR)/compn

clean:
	rm -f cwj cwj[0-9] cwjarm compn compn[0-9] *.o *.s out a.out incdir.h

test: install tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

# Run the tests with the
# compiler that compiled itself
test0: install tests/runtests0 cwj0
	(cd tests; chmod +x runtests0; ./runtests0)

# Run the tests with the
# compiler that compiled itself
test0n: install tests/runtests0n compn0
	(cd tests; chmod +x runtests0n; ./runtests0n)

armtest: cwjarm tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

testn: installn tests/runtestsn
	(cd tests; chmod +x runtestsn; ./runtestsn)

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

# Try to do the triple test with nasm
triplen: compn1
	size compn[01]

quadn: compn2
	size compn[012]

compn2: compn1 $(SRCN) $(HSRCS)
	./compn1 -o compn2 $(SRCN)

compn1: compn0 $(SRCN) $(HSRCS)
	./compn0 -o compn1 $(SRCN)

compn0: installn $(SRCN) $(HSRCS)
	echo "#define __NASM__ 1" >> incdir.h
	./compn  -o compn0 $(SRCN)
