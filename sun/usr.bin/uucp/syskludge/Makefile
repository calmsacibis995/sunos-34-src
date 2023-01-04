# syskludge.o comes last in following line
OBJ = access.o chdir.o chmod.o chown.o creat.o link.o open.o stat.o unlink.o \
	syskludge.o
TFILES = README INSTALL Makefile access.c chdir.c chmod.c chown.c \
	creat.c link.c open.c stat.c unlink.c syskludge.c testsys.c

.c.o:
	/lib/cpp -E $*.c > $*.s;  ${AS} $*.s -o $*.o
	-ld -x -r $*.o
	mv a.out $*.o; rm $*.s

syskludge.a:	$(OBJ)
	ar uv syskludge.a $(OBJ)
	ranlib syskludge.a

testsys:	testsys.o syskludge.a
	$(CC) -n testsys.o ../uucpdefs.o syskludge.a -o testsys

clean:
	rm -f $(OBJ) testsys.o testsys syskludge.a syskludge.tar

syskludge.o:	syskludge.c
	$(CC) -c -O syskludge.c

testsys.o:	testsys.c
	$(CC) -c -O testsys.c

syskludge.tar: $(TFILES)
	tar cbf 1 syskludge.tar $(TFILES)
