#include <stdio.h>
#include <stdlib.h>
#include "file_utils.h"
#include "node.h"

/**
 * Entry point for the application.
 * Initializes the root directory node, populates the directory structure,
 * processes changes, and frees resources.
 */
int main() {
    const char *dirpath = ".";  // Start directory

    // Create the root node for the directory
    Node *root = create_node(dirpath, FOLDER_NODE);

    // Populate the directory tree structure
    processDirectory(dirpath, root);

    // Print the directory tree
    printf("Tree Structure:\n");
    print_tree(root, 0);

    // Process the directory tree for changes
    processNode(root, dirpath);

    // Free the entire tree structure
    free_tree(root);

    return 0;
}
