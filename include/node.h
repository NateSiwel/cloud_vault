#ifndef NODE_H
#define NODE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LENGTH 256

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

Node *create_node(const char *name, NodeType type);
void add_child(Node *parent, Node *child);
void free_node(Node *node);
void free_tree(Node *node);
void print_tree(const Node *root, int depth);

#endif // NODE_H
