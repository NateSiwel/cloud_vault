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
    const char *dirpath = ".";

    // define root from disk or new  
    Node *root; 
    FILE *file = fopen("node_data.bin", "rb");
    if (!file) {
	printf("No saved directory tree, creating new.\n");
	root = create_node(dirpath, FOLDER_NODE);
	// Populates a node
	processDirectory(dirpath, root);
    } else {
	root = load_node(file);
	fclose(file);
    }

    //Handles changes to files via checksum
    processNode(root, dirpath);

    printf("Tree Structure:\n");
    print_tree(root, 0);

    // Save the node to file
    file = fopen("node_data.bin", "wb");
    if (!file) {
        perror("Failed to open file for writing");
        return 1;
    }
    save_node(file, root);
    fclose(file);

    // Free the original node
    free_tree(root);

    return 0;
}
