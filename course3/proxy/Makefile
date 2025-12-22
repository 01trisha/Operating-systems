CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -g
LDFLAGS = -pthread

SRCS = proxy.c cache.c http_parser.c picohttpparser.c
OBJS = $(SRCS:.c=.o)
TARGET = proxy

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

proxy.o: proxy.c cache.h http_parser.h
cache.o: cache.c cache.h
http_parser.o: http_parser.c http_parser.h picohttpparser.h
picohttpparser.o: picohttpparser.c picohttpparser.h

clean:
	rm -f $(OBJS) $(TARGET)
