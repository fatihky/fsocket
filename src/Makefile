LIBS=-lev
INCLUDE=-Ifsocket
FLAGS=-c -g -Wall -O3 -std=c99
CC=gcc $(INCLUDE) $(FLAGS) $(LIBS)

all: libfsocket

libfsocket: fsocket.o fstream.o anet.o sds.o zmalloc.o
	ar -rcs libfsocket.a fsocket.o fstream.o anet.o sds.o zmalloc.o

fsocket.o: fsocket.c
	$(CC) fsocket.c

fstream.o: fstream.c
	$(CC) fstream.c

anet.o: anet.c
	gcc -c -g -O3 -Wall -Ifsocket anet.c

sds.o: sds.c
	$(CC) sds.c

zmalloc.o: zmalloc.c
	$(CC) zmalloc.c

test: fsocket.o fstream.o anet.o sds.o zmalloc.o
	gcc test-server.c fsocket.o fstream.o anet.o sds.o zmalloc.o $(LIBS) -o test-server -g
	gcc test-client.c fsocket.o fstream.o anet.o sds.o zmalloc.o $(LIBS) -o test-client -g

install:
	cp -a fsocket /usr/local/include
	cp libfsocket.a /usr/lib

uninstall:
	rm -rf /usr/local/include/fsocket
	rm /usr/lib/libfsocket.a

clean:
	rm *.o
	rm libfsocket.a
	rm test-client
	rm test-server