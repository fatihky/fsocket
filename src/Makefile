CFLAGS=-g
LDFLAGS=-lev
C_FILES=$(wildcard *.c) $(wildcard utils/*.c) $(wildcard utils/libhl/*.c)
OBJ_LIST=$(patsubst %.c,%.o,$(C_FILES))

# examples
EXAMPLE_FILES := $(wildcard ../examples/*.c)
EXAMPLE_BINS := $(patsubst %.c,%,$(wildcard ../examples/*.c))

all: libfsocket.a

libfsocket.a: $(OBJ_LIST)
	ar -rcs libfsocket.a *.o utils/*.o utils/libhl/*.o

example: $(OBJ_LIST) libfsocket.a $(EXAMPLE_BINS) ../../libev/ev.c

PRG_PREFIX=../examples/

.SECONDEXPANSION:
BIN = $(patsubst %$(PRG_PREFIX),%,$@)
FILE = $(patsubst $(PRG_PREFIX)%,$(PRG_PREFIX)%.c,$@)

$(EXAMPLE_BINS): $(EXAMPLE_FILES) $(OBJ_LIST)
	$(CC) $(FILE) -o $(BIN) libfsocket.a $(LDFLAGS) $(CFLAGS)

clean:
	rm -rf $(OBJ_LIST)
	rm -rf $(EXAMPLE_BINS)
	rm -rf libfsocket.a
