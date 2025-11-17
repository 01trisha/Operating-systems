CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -D_GNU_SOURCE
LDFLAGS = 
TARGET = libuthread.so
TEST = test_uthread

.PHONY: all clean test

all: $(TARGET) $(TEST)

$(TARGET): uthread.c uthread.h
	$(CC) $(CFLAGS) -fPIC -shared -o $@ uthread.c $(LDFLAGS)

$(TEST): test.c $(TARGET)
	$(CC) $(CFLAGS) -o $@ test.c -L. -luthread $(LDFLAGS) -Wl,-rpath,.

test: $(TEST)
	./$(TEST)

clean:
	rm -f $(TARGET) $(TEST) *.o

.PHONY: run
run: test
