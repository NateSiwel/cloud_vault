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

void free_node(Node *node) {
    if (!node) return;
    free(node->checksum);
    free(node->blob_id);
    free_node(node->child);
    free_node(node->sibling);
    free(node);
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
