
CC=gcc
CFLAGS=-Wall -Wextra 

all: primer-client primer-server

primer-server: src/primer-server.c src/primer-server.h
	$(CC) src/primer-server.c src/primer-server.h $(CFLAGS) -o bin/primer-server

primer-client: src/primer-client.c
	$(CC) src/primer-client.c $(CFLAGS) -o bin/primer-client
clean:
	rm bin/*
