LIBS=-lev -lhl -lpthread
INCLUDE=-I../include
FLAGS=-c -g -Wall -Wextra -O3 -fPIC -std=c99
CC=gcc $(INCLUDE) $(FLAGS) $(LIBS)

all: libfsocket

libfsocket: fsocket.o io_handlers.o anet.o sds.o zmalloc.o
	ar -rcs libfsocket.a fsocket.o io_handlers.o anet.o sds.o zmalloc.o

fsocket.o: fsocket.c
	$(CC) fsocket.c

io_handlers.o: io_handlers.c
	$(CC) io_handlers.c

anet.o: anet.c
	gcc -c -g -O3 -Wall -Ifsocket -fPIC anet.c

sds.o: sds.c
	$(CC) sds.c

zmalloc.o: zmalloc.c
	$(CC) zmalloc.c

examples: libfsocket
	gcc ../examples/server.c $(INCLUDE) -L. -I. -lfsocket -o ../examples/server -g $(LIBS)
	gcc ../examples/client.c $(INCLUDE) -L. -I. -lfsocket -o ../examples/client -g $(LIBS)

bench: libfsocket
	gcc ../examples/bench-server.c $(INCLUDE) -L. -I. -lfsocket -o ../examples/bench-server -g $(LIBS)
	gcc ../examples/bench-client.c $(INCLUDE) -L. -I. -lfsocket -o ../examples/bench-client -g $(LIBS)

install: libfsocket
	cp ../include/fsocket.h /usr/local/include
	cp libfsocket.a /usr/lib

uninstall:
	rm -rf /usr/local/include/fsocket
	rm /usr/lib/libfsocket.a

clean:
	rm -rf *.o
	rm -rf libfsocket.a
	rm -rf ../examples/client
	rm -rf ../examples/server
	rm -rf ../examples/bench-client
	rm -rf ../examples/bench-server