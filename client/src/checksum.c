#include "checksum.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *calculateChecksum(const char *filepath) {
  FILE *file = fopen(filepath, "rb");
  if (!file) {
    perror("Failed to open file for checksum calculation");
    return NULL;
  }

  EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
  if (!md_ctx) {
    fclose(file);
    perror("Failed to create EVP_MD_CTX");
    return NULL;
  }

  const EVP_MD *md = EVP_MD_fetch(NULL, "MD5", NULL);
  if (!md) {
    EVP_MD_CTX_free(md_ctx);
    fclose(file);
    perror("Failed to fetch MD5 algorithm");
    return NULL;
  }

  if (EVP_DigestInit_ex(md_ctx, md, NULL) <= 0) {
    EVP_MD_free((EVP_MD *)md);
    EVP_MD_CTX_free(md_ctx);
    fclose(file);
    perror("Failed to initialize MD5 digest");
    return NULL;
  }

  unsigned char data[1024];
  size_t bytes;
  while ((bytes = fread(data, 1, sizeof(data), file)) > 0) {
    if (EVP_DigestUpdate(md_ctx, data, bytes) <= 0) {
      EVP_MD_free((EVP_MD *)md);
      EVP_MD_CTX_free(md_ctx);
      fclose(file);
      perror("Failed to update MD5 digest");
      return NULL;
    }
  }

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len;
  if (EVP_DigestFinal_ex(md_ctx, hash, &hash_len) <= 0) {
    EVP_MD_free((EVP_MD *)md);
    EVP_MD_CTX_free(md_ctx);
    fclose(file);
    perror("Failed to finalize MD5 digest");
    return NULL;
  }

  EVP_MD_free((EVP_MD *)md);
  EVP_MD_CTX_free(md_ctx);
  fclose(file);

  char *result = malloc((hash_len * 2) + 1);
  if (!result) {
    perror("Failed to allocate memory for checksum");
    return NULL;
  }

  for (unsigned int i = 0; i < hash_len; i++) {
    sprintf(result + (i * 2), "%02x", hash[i]);
  }
  result[hash_len * 2] = '\0';

  return result;
}
