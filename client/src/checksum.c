#include "checksum.h"
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>

char *calculateChecksum(const char *filepath) {
  FILE *file = fopen(filepath, "rb");
  if (!file) {
    perror("Failed to open file for checksum calculation");
    return NULL;
  }

  MD5_CTX md5_ctx;
  MD5_Init(&md5_ctx);

  unsigned char data[1024];
  size_t bytes;
  while ((bytes = fread(data, 1, sizeof(data), file)) > 0) {
    MD5_Update(&md5_ctx, data, bytes);
  }

  unsigned char hash[MD5_DIGEST_LENGTH];
  MD5_Final(hash, &md5_ctx);

  fclose(file);

  char *result = malloc((MD5_DIGEST_LENGTH * 2) + 1);
  if (!result) {
    perror("Failed to allocate memory for checksum");
    return NULL;
  }

  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(result + (i * 2), "%02x", hash[i]);
  }
  result[MD5_DIGEST_LENGTH * 2] = '\0';

  return result;
}
