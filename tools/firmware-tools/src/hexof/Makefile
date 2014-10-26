CC = gcc
CFLAGS =
WFLAGS = -Wall -Werror

all: hexof

hexof: hexof.o
	$(CC) $(LDFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(WFLAGS) -c -o $@ $<

clean:
	rm -f hexof *.o
