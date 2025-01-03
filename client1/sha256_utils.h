// sha256_utils.h
#ifndef SHA256_UTILS_H
#define SHA256_UTILS_H

#include <openssl/sha.h>
#include <stdint.h>

// Tính toán hash SHA-256 của một tệp và lưu vào buffer được cung cấp (64 ký tự hex + null terminator)
int compute_file_sha256(const char *filename, char *output_buffer);

// Tính toán hash SHA-256 của một buffer và lưu vào buffer được cung cấp (64 ký tự hex + null terminator)
void compute_buffer_sha256(const unsigned char *buffer, size_t length, char *output_buffer);

// Chuyển đổi digest SHA256 thành chuỗi hex
void digest_to_hex(const unsigned char *digest, char *output_buffer);

#endif // SHA256_UTILS_H
