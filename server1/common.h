#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define PORT 8085

void handle_error(const char *message);
int send_all(int sock, const void *buffer, size_t length);
int recv_all(int sock, void *buffer, size_t length);
uint64_t htonll(uint64_t value);
uint64_t ntohll(uint64_t value);

#endif // COMMON_H
