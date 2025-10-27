CC = gcc
CFLAGS = -Wall -Wextra -fPIC
LIB = libmythread.so

all: $(LIB) test

$(LIB): mythread.o
	$(CC) -shared -o $@ $^

mythread.o: mythread.c mythread.h
	$(CC) $(CFLAGS) -c mythread.c -o mythread.o

test: test_mythread.o $(LIB)
	$(CC) -o test test.o -L. -lmythread -Wl,-rpath=.

test_mythread.o: test.c mythread.h
	$(CC) $(CFLAGS) -c test.c -o test.o

clean:
	rm -f *.o *.so test

