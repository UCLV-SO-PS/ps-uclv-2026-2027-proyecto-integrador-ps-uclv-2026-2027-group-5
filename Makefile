CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu11
AR = ar
ARFLAGS = rcs

all: libmonitor.a server client

libmonitor.a: monitor.o
	$(AR) $(ARFLAGS) $@ $^

server: server.o libmonitor.a
	$(CC) $(CFLAGS) -o $@ server.o -L. -lmonitor

client: client.o
	$(CC) $(CFLAGS) -o $@ client.o

monitor.o: monitor.c monitor.h
	$(CC) $(CFLAGS) -c monitor.c -o monitor.o

server.o: server.c monitor.h
	$(CC) $(CFLAGS) -c server.c -o server.o

client.o: client.c
	$(CC) $(CFLAGS) -c client.c -o client.o

clean:
	rm -f *.o libmonitor.a server client

.PHONY: all clean