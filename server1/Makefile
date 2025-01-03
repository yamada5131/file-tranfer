# Makefile_server

CC = gcc
CFLAGS = -Wall -g  # Thêm cờ -g để hỗ trợ debug
LDFLAGS = -lssl -lcrypto  # Thêm các thư viện OpenSSL cần thiết
DEPS = common.h sha256_utils.h
OBJ = server.o functions.o sha256_utils.o

all: server

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o server
