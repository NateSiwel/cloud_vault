#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stddef.h>
#include "node.h"

#define MAX_PATH 1024
#define MAX_FILE_SIZE 1048576

/* Populates a parent node with directory contents */
void processDirectory(const char *dirpath, Node *parent);

/* Reads the contents of a file into memory */
char *readFileContents(const char *filepath, size_t *size);

/* Uploads a file to the server and updates its metadata */
void uploadFile(Node *node, const char *filepath);

/* Processes a node recursively, checking for file changes */
void processNode(Node *node, const char *currentPath);

#endif // FILE_UTILS_H
