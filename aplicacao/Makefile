CC = gcc
CFLAGS = -Wall -g 
DEPS = common.h cache.h document.h search.h handlers.h utils.h dserver.h dclient.h
OBJ_SERVER = dserver.o cache.o document.o search.o handlers.o utils.o
OBJ_CLIENT = dclient.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: dserver dclient

dserver: $(OBJ_SERVER)
	$(CC) -o $@ $^ $(CFLAGS)

dclient: $(OBJ_CLIENT)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o dserver dclient