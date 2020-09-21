CC=gcc
CFLAGS=-Wall -Werror -O2

PROG=ponyo

$(PROG): ponyo.c

test: $(PROG)
	@./runtests.sh

clean:
	rm -f $(PROG)
