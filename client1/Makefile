# Makefile_client

CC = gcc
CFLAGS = -Wall -g  # Thêm cờ -g để hỗ trợ debug
DEPS = common.h
OBJ = client.o functions.o

all: client

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o client
