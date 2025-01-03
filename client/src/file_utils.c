#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "file_utils.h"
#include "node.h"
#include "checksum.h"

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

void processTree(const char *dirpath, Node *node) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    // Mark all existing child nodes as deleted initially
    Node *current = node->child;
    while (current) {
        current->is_deleted = 1;
        current = current->sibling;
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

        Node *current = node->child;
        Node *found = NULL;
        while (current) {
            if (strcmp(current->name, entry->d_name) == 0) {
                found = current;
                break;
            }
            current = current->sibling;
        }

        if (S_ISREG(file_stat.st_mode)) {
            char *new_checksum = calculateChecksum(filepath);
            if (found) {
                // File already exists, mark as not deleted
                found->is_deleted = 0;

                // Compare checksums
                if (found->checksum && strcmp(found->checksum, new_checksum) != 0) {
                    printf("File changed: %s\n", filepath);
                    free(found->checksum);
                    found->checksum = new_checksum;
                } else {
                    free(new_checksum);
                }
            } else {
                // Add new file node
                Node *file_node = create_node(entry->d_name, FILE_NODE);
                file_node->checksum = new_checksum;
                file_node->is_deleted = 0;
                add_child(node, file_node);
            }
        } else if (S_ISDIR(file_stat.st_mode)) {
            if (found) {
                // Directory already exists, mark as not deleted
                found->is_deleted = 0;
                processTree(filepath, found);
            } else {
                // Add new folder node
                Node *folder_node = create_node(entry->d_name, FOLDER_NODE);
                folder_node->is_deleted = 0;
                add_child(node, folder_node);
                processTree(filepath, folder_node);
            }
        }
    }

    closedir(dir);

    // Remove any nodes still marked as deleted
    Node *prev = NULL;
    Node *child = node->child;
    while (child) {
        if (child->is_deleted) {
            printf("File or directory deleted: %s\n", child->name);
            Node *to_delete = child;

            if (prev) {
                prev->sibling = child->sibling;
            } else {
                node->child = child->sibling;
            }

            child = child->sibling;
            free_tree(to_delete);
        } else {
            prev = child;
            child = child->sibling;
        }
    }
}

