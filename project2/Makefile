
CC=gcc
CFLAGS=-Wall -Wextra 

all: router sender receiver


router: src/project.h src/router.c
	$(CC) src/router.c src/project.h $(CFLAGS) -o bin/router

sender: src/project.h src/sender.c
	$(CC) src/sender.c src/project.h $(CFLAGS) -o bin/sender

receiver: src/project.h src/sender.c
	$(CC) src/receiver.c src/project.h $(CFLAGS) -o bin/receiver


clean:
	rm bin/*
	rm output/*
