#include "node.h"

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
    new_node->checksum = NULL;
    new_node->blob_id = NULL;
    new_node->is_uploaded = 0;
    new_node->is_deleted=0;

    return new_node;
}

void add_child(Node *parent, Node *child) {
    if (!parent || parent->type != FOLDER_NODE) {
        fprintf(stderr, "Cannot add child to non-folder node\n");
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

void write_string(FILE *file, const char *str) {
    size_t length = str ? strlen(str) : 0;
    fwrite(&length, sizeof(size_t), 1, file);
    if (length > 0) {
        fwrite(str, sizeof(char), length, file);
    }
}

char *read_string(FILE *file) {
    size_t length;
    fread(&length, sizeof(size_t), 1, file);
    if (length == 0) {
        return NULL;
    }
    char *str = (char *)malloc(length + 1);
    fread(str, sizeof(char), length, file);
    str[length] = '\0';
    return str;
}

void save_node(FILE *file, Node *node) {
    if (node == NULL) {
        int null_flag = 1;
        fwrite(&null_flag, sizeof(int), 1, file);
        return;
    }
    int null_flag = 0;
    fwrite(&null_flag, sizeof(int), 1, file);

    fwrite(node->name, sizeof(char), MAX_NAME_LENGTH, file);
    fwrite(&node->type, sizeof(NodeType), 1, file);
    write_string(file, node->checksum);
    write_string(file, node->blob_id);
    fwrite(&node->is_uploaded, sizeof(int), 1, file);

    save_node(file, node->child);
    save_node(file, node->sibling);
}

Node *load_node(FILE *file) {
    int null_flag;
    fread(&null_flag, sizeof(int), 1, file);
    if (null_flag) {
        return NULL;
    }

    Node *node = (Node *)malloc(sizeof(Node));
    fread(node->name, sizeof(char), MAX_NAME_LENGTH, file);
    fread(&node->type, sizeof(NodeType), 1, file);
    node->checksum = read_string(file);
    node->blob_id = read_string(file);
    fread(&node->is_uploaded, sizeof(int), 1, file);

    node->child = load_node(file);
    node->sibling = load_node(file);

    return node;
}

void free_node(Node *node) {
    if (node == NULL) {
        return;
    }
    free(node->checksum);
    free(node->blob_id);
    free_node(node->child);
    free_node(node->sibling);
    free(node);
}
