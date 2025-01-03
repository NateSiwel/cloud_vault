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
    int is_deleted;
    struct Node *child;
    struct Node *sibling;
} Node;

Node *create_node(const char *name, NodeType type);
void add_child(Node *parent, Node *child);
void free_node(Node *node);
void free_tree(Node *node);
void print_tree(const Node *root, int depth);
void write_string(FILE *file, const char *str);
char *read_string(FILE *file);
void save_node(FILE *file, Node *node);
Node *load_node(FILE *file);
void free_node(Node *node);

#endif // NODE_H
