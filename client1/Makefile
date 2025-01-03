# Makefile_client

CC = gcc
CFLAGS = -Wall -g  # Thêm cờ -g để hỗ trợ debug
LDFLAGS = -lssl -lcrypto  # Thêm các thư viện OpenSSL cần thiết
DEPS = common.h sha256_utils.h
OBJ = client.o functions.o sha256_utils.o

all: client

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o client
