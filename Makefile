CC=gcc
CFLAGS=-Wall -Werror

DEPS=error.h eval.h lexer.h parser.h
OBJS=eval.o lexer.o parser.o ponyo.o
PROG=ponyo

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

test: CFLAGS += -DPONYO_TEST
test: clean $(PROG)
	@./runtests.sh && make clean

clean:
	rm -f $(OBJS) $(PROG)
