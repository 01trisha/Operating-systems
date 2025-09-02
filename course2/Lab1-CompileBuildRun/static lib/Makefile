compile:
	gcc -c libstatic.c -o libstatic.o
	ar r libstatic.a libstatic.o
	gcc hello.c -L. -lstatic -o hello
	rm *.o
clean:
	rm -f *.o *.a hello