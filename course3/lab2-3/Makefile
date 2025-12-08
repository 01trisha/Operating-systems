CC=gcc
CFLAGS=-Wall -Wextra -pthread -O2
TARGET=lab2-3

all: $(TARGET)

$(TARGET): main.o storage.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o storage.o

main.o: main.c storage.h
	$(CC) $(CFLAGS) -c main.c

storage.o: storage.c storage.h
	$(CC) $(CFLAGS) -c storage.c

clean:
	rm -f *.o $(TARGET)

test: $(TARGET)
	@echo "Testing with mutex, size 100..."
	./$(TARGET) 100 0 &
	@sleep 5
	@killall -INT $(TARGET) 2>/dev/null || true
	@echo ""
	@echo "Testing with spinlock, size 100..."
	./$(TARGET) 100 1 &
	@sleep 5
	@killall -INT $(TARGET) 2>/dev/null || true
	@echo ""
	@echo "Testing with rwlock, size 100..."
	./$(TARGET) 100 2 &
	@sleep 5
	@killall -INT $(TARGET) 2>/dev/null || true

.PHONY: all clean test
