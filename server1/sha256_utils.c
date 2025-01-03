// sha256_utils.c
#include "sha256_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Chuyển đổi digest SHA256 thành chuỗi hex
void digest_to_hex(const unsigned char *digest, char *output_buffer) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(output_buffer + (i * 2), "%02x", digest[i]);
    }
    output_buffer[64] = '\0'; // Kết thúc chuỗi
}

// Tính toán hash SHA-256 của một tệp
int compute_file_sha256(const char *filename, char *output_buffer) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Không thể mở tệp để tính toán hash");
        return -1;
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int bufSize = 32768;
    unsigned char *buffer = malloc(bufSize);
    if (!buffer) {
        fclose(file);
        return -1;
    }
    int bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, bufSize, file))) {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_Final(digest, &sha256);
    digest_to_hex(digest, output_buffer);
    fclose(file);
    free(buffer);
    return 0;
}

// Tính toán hash SHA-256 của một buffer
void compute_buffer_sha256(const unsigned char *buffer, size_t length, char *output_buffer) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, buffer, length);
    SHA256_Final(digest, &sha256);
    digest_to_hex(digest, output_buffer);
}
