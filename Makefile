CC=gcc
CFLAGS=-Wall -Werror

DEPS=error.h
OBJS=ponyo.o
PROG=ponyo

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJS) $(PROG)
