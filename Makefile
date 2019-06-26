CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wno-unused-variable

default: simpsh

simpsh: simpsh.c simpsh.h
	$(CC) $(CFLAGS) simpsh.c -o $@

check: simpsh
	./test.sh

distname = lab1-NathanChouNathanielChu
distfiles = simpsh.c simpsh.h Makefile test.sh README
dist: $(distname).tar.gz
$(distname).tar.gz: $(distfiles)
	./test.sh && tar -czf $@ --transform "s,^,$(distname)/," $(distfiles)

.PHONY: default clean check dist

clean:
	rm -f *.tar.gz simpsh test*.txt
