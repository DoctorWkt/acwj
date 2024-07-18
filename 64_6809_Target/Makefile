# Define the location of the include directory
# and the location to install the compiler binary.
# You will need to make TOPDIR and make it writable by you.
#
TOPDIR=/opt/wcc
BINDIR=$(TOPDIR)/bin
INCQBEDIR=$(TOPDIR)/include/qbe
INC6809DIR=$(TOPDIR)/include/6809
LIB6809DIR=$(TOPDIR)/lib/6809

CFLAGS=-g -Wall
# CFLAGS+= --coverage
# then gcov *gcno

# Header files and C files for the QBE and 6809 parser phase
#
PARSEH= cg.h data.h decl.h defs.h expr.h gen.h misc.h opt.h \
	parse.h stmt.h sym.h target.h tree.h types.h
PARSEC6809= decl.c expr.c misc.c opt.c parse.c stmt.c sym.c tree.c \
	targ6809.c tstring.c types.c
PARSECQBE= decl.c expr.c misc.c opt.c parse.c stmt.c sym.c tree.c \
	targqbe.c tstring.c types.c

# Header files and C files for the QBE and 6809 code generator phase
#
GENH= cg.h data.h defs.h gen.h misc.h sym.h target.h tree.h types.h
GENC6809= cg6809.c cgen.c gen.c misc.c sym.c targ6809.c tree.c types.c
GENCQBE= cgqbe.c cgen.c gen.c misc.c sym.c targqbe.c tree.c types.c

# These executables are compiled by the existing C compiler on your system.
#
all: wcc cscan detok detree desym cpeep \
	cparse6809 cgen6809 cparseqbe cgenqbe

wcc: wcc.c wcc.h l0dirs.h
	cc -o wcc $(CFLAGS) wcc.c

cscan: scan.c defs.h misc.h misc.c
	cc -o cscan $(CFLAGS) scan.c misc.c

cpeep: cpeep.c
	cc -o cpeep $(CFLAGS) cpeep.c

cparse6809: $(PARSEC6809) $(PARSEH)
	cc -o cparse6809 $(CFLAGS) -DWRITESYMS $(PARSEC6809)

cgen6809: $(GENC6809) $(GENH)
	cc -o cgen6809 $(CFLAGS) $(GENC6809)

cparseqbe: $(PARSECQBE) $(PARSEH)
	cc -o cparseqbe $(CFLAGS) -DWRITESYMS $(PARSECQBE)

cgenqbe: $(GENCQBE) $(GENH)
	cc -o cgenqbe $(CFLAGS) $(GENCQBE)

desym: desym.c defs.h types.h
	cc -o desym $(CFLAGS) desym.c

detok: detok.c tstring.c defs.h
	cc -o detok $(CFLAGS) detok.c tstring.c

detree: detree.c misc.c tree.c misc.h defs.h tree.h
	cc -o detree $(CFLAGS) -DDETREE detree.c misc.c tree.c

l0dirs.h:
	echo "#define TOPDIR \"$(TOPDIR)\"" > l0dirs.h
	echo "#define INCQBEDIR \"$(INCQBEDIR)\"" >> l0dirs.h
	echo "#define INC6809DIR \"$(INC6809DIR)\"" >> l0dirs.h
	echo "#define BINDIR \"$(BINDIR)\"" >> l0dirs.h
	echo "#define LIB6809DIR \"$(LIB6809DIR)\"" >> l0dirs.h
	cp l0dirs.h dirs.h

# Install the compiler built by the external compiler
#
install: all
	@if [ ! -d $(TOPDIR) ]; then echo "$(TOPDIR) doesn't exit, create it writeable by you"; exit 1; fi
	mkdir -p $(INC6809DIR)
	mkdir -p $(INCQBEDIR)
	mkdir -p $(BINDIR)
	mkdir -p $(LIB6809DIR)
	rsync -a --exclude RCS include/qbe/. $(INCQBEDIR)
	rsync -a --exclude RCS include/6809/. $(INC6809DIR)
	rsync -a --exclude Makefile --exclude RCS --exclude crt0.s \
		lib/6809/. $(LIB6809DIR)
	cp wcc cscan detok detree desym cpeep \
	  cparse6809 cgen6809 \
	  cparseqbe cgenqbe $(BINDIR)

# These rules are for the compiler to build itself.
# Use the L1 directory to hold the binaries.
#
l1dirs.h: TOPDIR= `pwd`
l1dirs.h: BINDIR= `pwd`/L1

l1dirs.h:
	echo "#define TOPDIR \"$(TOPDIR)\"" > l1dirs.h
	echo "#define INCQBEDIR \"$(INCQBEDIR)\"" >> l1dirs.h
	echo "#define INC6809DIR \"$(INC6809DIR)\"" >> l1dirs.h
	echo "#define BINDIR \"$(BINDIR)\"" >> l1dirs.h
	echo "#define LIB6809DIR \"$(LIB6809DIR)\"" >> l1dirs.h
	cp l1dirs.h dirs.h

# These executables are compiled by our own compiler
#
l1bins: install L1/wcc L1/cscan L1/cparseqbe L1/cgenqbe \
		L1/desym L1/detok L1/detree

L1/wcc: wcc.c wcc.h l1dirs.h
	mkdir -p L1
	wcc -o L1/wcc wcc.c

L1/cscan: scan.c defs.h misc.h misc.c
	wcc -o L1/cscan scan.c misc.c

L1/cparseqbe: $(PARSECQBE) $(PARSEH)
	wcc -o L1/cparseqbe -DWRITESYMS $(PARSECQBE)

L1/cgenqbe: $(GENCQBE) $(GENH)
	wcc -o L1/cgenqbe $(GENCQBE)

L1/desym: desym.c defs.h types.h
	wcc -o L1/desym desym.c

L1/detok: detok.c tstring.c defs.h
	wcc -o L1/detok detok.c tstring.c

L1/detree: detree.c misc.c tree.c misc.h defs.h tree.h
	wcc -o L1/detree -DDETREE detree.c misc.c tree.c

# These rules are for the compiler to build itself a second time.
# If the binaries match those built the first time, we know that
# the compiler can successfully compile itself.
# Use the L2 directory to hold the binaries.
#
l2dirs.h: TOPDIR= `pwd`
l2dirs.h: BINDIR= `pwd`/L2

l2dirs.h:
	echo "#define TOPDIR \"$(TOPDIR)\"" > l2dirs.h
	echo "#define INCQBEDIR \"$(INCQBEDIR)\"" >> l2dirs.h
	echo "#define INC6809DIR \"$(INC6809DIR)\"" >> l2dirs.h
	echo "#define BINDIR \"$(BINDIR)\"" >> l2dirs.h
	echo "#define LIB6809DIR \"$(LIB6809DIR)\"" >> l2dirs.h
	cp l2dirs.h dirs.h

# These executables are compiled by our own compiler
#
l2bins: l1bins L2/wcc L2/cscan L2/cparseqbe L2/cgenqbe \
		L2/desym L2/detok L2/detree

L2/wcc: wcc.c wcc.h l2dirs.h
	mkdir -p L2
	L1/wcc -o L2/wcc wcc.c

L2/cscan: scan.c defs.h misc.h misc.c
	L1/wcc -o L2/cscan scan.c misc.c

L2/cparseqbe: $(PARSECQBE) $(PARSEH)
	L1/wcc -o L2/cparseqbe -DWRITESYMS $(PARSECQBE)

L2/cgenqbe: $(GENCQBE) $(GENH)
	L1/wcc -o L2/cgenqbe $(GENCQBE)

L2/desym: desym.c defs.h types.h
	L1/wcc -o L2/desym desym.c

L2/detok: detok.c tstring.c defs.h
	L1/wcc -o L2/detok detok.c tstring.c

L2/detree: detree.c misc.c tree.c misc.h defs.h tree.h
	L1/wcc -o L2/detree -DDETREE detree.c misc.c tree.c

# Do the triple test: build the compiler with the external compiler,
# build the compiler with itself and then use this compiler to
# build the compiler again. The binaries should be identical. Note
# that the `wcc` binaries are different because the TOPDIR is different.
#
triple: l2bins
	md5sum L1/* L2/* | sort

# Clean up all versions of the compiler
clean:
	rm -f wcc cscan detok detree desym cpeep \
	  cparse6809 cgen6809 \
	  cparseqbe cgenqbe 
	rm -f *.o *.s out a.out dirs.h l?dirs.h *.gc??
	rm -rf L1 L2

# Run the tests with the compiler built with the external compiler
#
test: install tests/runtests
	(cd tests; chmod +x runtests; ./runtests qbe)

tests: test

# Run the 6809 tests
6test: install tests/runtests
	(cd tests; chmod +x runtests; ./runtests 6809)
