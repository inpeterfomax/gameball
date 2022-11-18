# _*_ MakeFile _*_

EEXEC = gameball
CC = gcc
CFLAGS = -c
LIBRARY_PATH = /usr/lib
INCLUDE_PATH = /usr/include
LIBS = X11


$(EEXEC) : gameball.o
	$(CC) -L$(LIBRARY_PATH) gameball.o -o $(EEXEC) -l$(LIBS)

gameball.o: gameball.c
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) gameball.c 
