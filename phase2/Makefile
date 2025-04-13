CC = gcc
CFLAGS = -Og
LDLIBS = -lpthread

PROGS = myShell

all: $(PROGS)

myShell: myshell.c csapp.c
	$(CC) $(CFLAGS) -o myShell myshell.c csapp.c $(LDLIBS)

clean:
	rm -rf *~ $(PROGS)