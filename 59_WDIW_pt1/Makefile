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
	rm -f cwj cwj0 cwj1 cwjarm compn compn0 compn1 *.o *.s out a.out incdir.h

test: install tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

armtest: cwjarm tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

testn: installn tests/runtestsn
	(cd tests; chmod +x runtestsn; ./runtestsn)

# Try to do the triple test
triple: cwj1
	size cwj[01]

cwj1: cwj0 $(SRCS) $(HSRCS)
	./cwj0 -o cwj1 $(SRCS)

cwj0: install $(SRCS) $(HSRCS)
	./cwj  -o cwj0 $(SRCS)

# Try to do the triple test with nasm
triplen: compn1
	size compn[01]

compn1: compn0 $(SRCN) $(HSRCS)
	./compn0 -o compn1 $(SRCN)

compn0: installn $(SRCN) $(HSRCS)
	./compn  -o compn0 $(SRCN)
