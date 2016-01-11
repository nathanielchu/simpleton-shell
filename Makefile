CC = gcc
CFLAGS = -std=c11 -Wall -Wextra

default: simpsh

simpsh: simpsh.c simpsh.h
	$(CC) $(CFLAGS) simpsh.c -o $@

.PHONY: default clean check dist

clean:
	rm -f *.tar.gz simpsh
