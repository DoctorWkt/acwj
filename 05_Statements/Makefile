comp1: cg.c expr.c gen.c main.c misc.c scan.c stmt.c tree.c
	cc -o comp1 -g cg.c expr.c gen.c main.c misc.c scan.c stmt.c tree.c

clean:
	rm -f comp1 *.o *.s out

test: comp1 input01
	./comp1 input01
	cc -o out out.s
	./out
