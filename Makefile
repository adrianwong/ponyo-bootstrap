CC=gcc
CFLAGS=-Wall -Werror

DEPS=error.h eval.h lexer.h parser.h
OBJS=eval.o lexer.o parser.o ponyo.o
PROG=ponyo

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJS) $(PROG)
