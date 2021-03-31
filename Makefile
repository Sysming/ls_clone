CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -D_GNU_SOURCE
EXEC=ls

all : $(EXEC)

$(EXEC) : ls.o
	$(CC) $^ -o $@

ls.o : ls.c
	$(CC) -c $(CFLAGS) $^ -o $@

clean : 
	rm -f *.o
	rm $(EXEC)
