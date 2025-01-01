#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/md5.h>

#define MAX_NAME_LENGTH 256
#define MAX_PATH 1024
#define MAX_FILE_SIZE 1048576

typedef enum { FILE_NODE, FOLDER_NODE } NodeType;

typedef struct Node {
    char name[MAX_NAME_LENGTH];
    NodeType type;
    char *checksum;
    char *blob_id;
    int is_uploaded;
    struct Node *child;
    struct Node *sibling;
} Node;

char *calculateChecksum(const char *filepath);
void uploadFile(Node *node, const char *filepath);

Node *create_node(const char *name, NodeType type) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node) {
        perror("Failed to allocate memory for node");
        exit(EXIT_FAILURE);
    }
    strncpy(new_node->name, name, MAX_NAME_LENGTH);
    new_node->type = type;
    new_node->child = NULL;
    new_node->sibling = NULL;
    new_node->blob_id = NULL;
    new_node->is_uploaded = 0;
    new_node->checksum = NULL;

    return new_node;
}

void add_child(Node *parent, Node *child) {
    if (parent->type != FOLDER_NODE) {
        fprintf(stderr, "Cannot add a child to a file node\n");
        return;
    }
    if (!parent->child) {
        parent->child = child;
    } else {
        Node *temp = parent->child;
        while (temp->sibling) {
            temp = temp->sibling;
        }
        temp->sibling = child;
    }
}

void processNode(Node *node, const char *currentPath) {
    if (!node) return;

    // Construct the full path of the current node
    char fullPath[MAX_PATH];
	
    snprintf(fullPath, MAX_PATH, "%s/%s", currentPath, node->name);

    if (node->type == FILE_NODE) {

        // Calculate the file's current checksum
        char *current_checksum = calculateChecksum(fullPath);

        if (!current_checksum) {
            fprintf(stderr, "Checksum calculation failed for %s\n", fullPath);
            return;
        }

        // Check if the file has changed or is new
        if (!node->is_uploaded || !node->checksum || strcmp(node->checksum, current_checksum) != 0) {
            uploadFile(node, fullPath);
        } else {
            printf("File is unchanged: %s\n", fullPath);
        }

        free(current_checksum);  // Free temporary checksum
    } else if (node->type == FOLDER_NODE) {
        // Process child nodes
        Node *child = node->child;
        while (child) {
            processNode(child, fullPath);
            child = child->sibling;
        }
    }
}

void uploadFile(Node *node, const char *filepath) {
    // Placeholder for uploading the file to the server
    printf("Uploading file: %s\n", filepath);

    // Simulate successful upload
    node->is_uploaded = 1;

    // Recalculate and store the checksum
    char *new_checksum = calculateChecksum(filepath);
    if (new_checksum) {
        free(node->checksum);  // Free old checksum if it exists
        node->checksum = new_checksum;
    }

    printf("%s\n\n", new_checksum);

    // Simulate assigning a server blob ID
    node->blob_id = strdup("unique_blob_id");  // Replace with server-generated ID
}


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
                //printf("File content (%zu bytes):\n%s\n\n", size, content);
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

void print_tree(const Node *root, int depth) {
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    printf("|-%s (%s)\n", root->name, root->type == FILE_NODE ? "File" : "Folder");
    if (root->type == FOLDER_NODE) {
        Node *child = root->child;
        while (child) {
            print_tree(child, depth + 1);
            child = child->sibling;
        }
    }
}

void free_tree(Node *root) {
    if (!root) return;
    if (root->type == FOLDER_NODE) {
        Node *child = root->child;
        while (child) {
            Node *next = child->sibling;
            free_tree(child);
            child = next;
        }
    }
    free(root);
}

char *calculateChecksum(const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Failed to open file for checksum calculation");
        return NULL;
    }

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);

    unsigned char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        MD5_Update(&md5_ctx, buffer, bytesRead);
    }

    fclose(file);

    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_Final(hash, &md5_ctx);

    // Convert the hash to a readable hexadecimal string
    char *checksum = malloc(MD5_DIGEST_LENGTH * 2 + 1);
    if (!checksum) {
        perror("Failed to allocate memory for checksum");
        return NULL;
    }
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&checksum[i * 2], "%02x", hash[i]);
    }

    return checksum;
}

int main() {


    const char *dirpath = "."; 

    Node *root = create_node(dirpath, FOLDER_NODE);

    processDirectory(".", root);

    printf("Tree Structure:\n");
    print_tree(root, 0);


    processNode(root, dirpath);


    free_tree(root);

    return 0;
}
