CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
TARGET = main

all: $(TARGET)

$(TARGET): main.o queue.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o queue.o

main.o: main.c queue.h
	$(CC) $(CFLAGS) -c main.c

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

clean:
	rm -f *.o $(TARGET)

run: $(TARGET)
	./$(TARGET)

run100: $(TARGET)
	./$(TARGET) 100

run1000: $(TARGET)
	./$(TARGET) 1000

run10000: $(TARGET)
	./$(TARGET) 10000

run100000: $(TARGET)
	./$(TARGET) 100000

.PHONY: all clean run run100 run1000 run10000 run100000
