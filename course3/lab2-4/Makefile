CC = gcc
CFLAGS = -Wall -pthread -std=c11

all: test_sync test_performance

test_sync: test_sync.o spinlock.o mutex.o
	$(CC) $(CFLAGS) -o test_sync test_sync.o spinlock.o mutex.o

test_performance: test_performance.o mutex.o
	$(CC) $(CFLAGS) -o test_performance test_performance.o mutex.o

test_performance.o: test_performance.c mutex.h
	$(CC) $(CFLAGS) -c test_performance.c

test_sync.o: test_sync.c spinlock.h mutex.h
	$(CC) $(CFLAGS) -c test_sync.c

spinlock.o: spinlock.c spinlock.h
	$(CC) $(CFLAGS) -c spinlock.c

mutex.o: mutex.c mutex.h
	$(CC) $(CFLAGS) -c mutex.c

clean:
	rm -f *.o test_sync test_performance

.PHONY: all clean
