CC=gcc
CFLAGS=-Wall -Werror

PROG=ponyo

$(PROG): ponyo.c

test: $(PROG)
	@./runtests.sh

clean:
	rm -f $(PROG)
