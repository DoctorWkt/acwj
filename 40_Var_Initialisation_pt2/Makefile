# Define the location of the include directory
# and the location to install the compiler binary
INCDIR=/tmp/include
BINDIR=/tmp

HSRCS= data.h decl.h defs.h
SRCS= cg.c decl.c expr.c gen.c main.c misc.c \
	scan.c stmt.c sym.c tree.c types.c

SRCN= cgn.c decl.c expr.c gen.c main.c misc.c \
	scan.c stmt.c sym.c tree.c types.c

ARMSRCS= cg_arm.c decl.c expr.c gen.c main.c misc.c \
	scan.c stmt.c sym.c tree.c types.c

cwj: $(SRCS) $(HSRCS)
	cc -o cwj -g -Wall -DINCDIR=\"$(INCDIR)\" $(SRCS)

compn: $(SRCN) $(HSRCS)
	cc -D__NASM__ -o compn -g -Wall -DINCDIR=\"$(INCDIR)\" $(SRCN)

cwjarm: $(ARMSRCS) $(HSRCS)
	cc -o cwjarm -g -Wall $(ARMSRCS)
	cp cwjarm cwj

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
	rm -f cwj cwjarm compn *.o *.s out a.out

test: install tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

armtest: cwjarm tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

testn: installn tests/runtestsn
	(cd tests; chmod +x runtestsn; ./runtestsn)
