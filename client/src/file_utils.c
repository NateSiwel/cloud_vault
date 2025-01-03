#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "file_utils.h"
#include "node.h"
#include "checksum.h"

/* Populates parent node with content in dirpath */
void processDirectory(const char *dirpath, Node *parent) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char filepath[MAX_PATH];
        snprintf(filepath, MAX_PATH, "%s/%s", dirpath, entry->d_name);

        struct stat file_stat;
        if (stat(filepath, &file_stat) == -1) {
            perror("Failed to get file stats");
            continue;
        }

        if (S_ISREG(file_stat.st_mode)) {
            printf("Processing file: %s\n", filepath);

            size_t size;
            char *content = readFileContents(filepath, &size);
            if (content) {
                free(content);
            }

            Node *file_node = create_node(entry->d_name, FILE_NODE);
            add_child(parent, file_node);
        } else if (S_ISDIR(file_stat.st_mode)) {
            printf("Entering directory: %s\n", filepath);

            Node *folder_node = create_node(entry->d_name, FOLDER_NODE);
            add_child(parent, folder_node);

            processDirectory(filepath, folder_node);
        }
    }

    closedir(dir);
}

/* Reads file contents using fread */
char *readFileContents(const char *filepath, size_t *size) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Failed to open file!");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);

    if (*size > MAX_FILE_SIZE) {
        fprintf(stderr, "File %s exceeds max supported size.\n", filepath);
        fclose(file);
        return NULL;
    }

    char *content = (char *)malloc(*size + 1);
    if (!content) {
        perror("Could not allocate memory");
        fclose(file);
        return NULL;
    }

    fread(content, 1, *size, file);
    content[*size] = '\0';
    fclose(file);

    return content;
}

/* Uploads file to server and updates the node */
void uploadFile(Node *node, const char *filepath) {
    printf("Uploading file: %s\n", filepath);

    node->is_uploaded = 1;

    char *new_checksum = calculateChecksum(filepath);
    if (new_checksum) {
        free(node->checksum);
        node->checksum = new_checksum;
    }
    printf("Checksum: %s\n", new_checksum);

    node->blob_id = strdup("unique_blob_id");  // Replace with server-generated ID
}

/* Processes a node recursively, checking for changes */
void processNode(Node *node, const char *currentPath) {
    if (!node) return;

    char fullPath[MAX_PATH];
    snprintf(fullPath, MAX_PATH, "%s/%s", currentPath, node->name);

    if (node->type == FILE_NODE) {
        char *current_checksum = calculateChecksum(fullPath);
        if (!current_checksum) {
            fprintf(stderr, "Checksum calculation failed for %s\n", fullPath);
            return;
        }

        if (!node->is_uploaded || !node->checksum || strcmp(node->checksum, current_checksum) != 0) {
            uploadFile(node, fullPath);
        } else {
            printf("File is unchanged: %s\n", fullPath);
        }

        free(current_checksum);
    } else if (node->type == FOLDER_NODE) {
        Node *child = node->child;
        while (child) {
            processNode(child, fullPath);
            child = child->sibling;
        }
    }
}
