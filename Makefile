CC=gcc
CFLAGS=-Wall -Werror

OBJS=ponyo.o
PROG=ponyo

%.o: %.c
	$(CC) -c -o $@ $<

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJS) $(PROG)
