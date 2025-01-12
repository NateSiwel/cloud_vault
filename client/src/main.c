#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "file_utils.h"
#include "node.h"

#define SERVER_IP "127.0.0.1"
#define PORT 8080

/* 
 * Entry point for client-side backup logic. 
 * Requires active server running @ SERVER_IP:PORT
 */
int main() {

  // Establish socket and connection with server
  int server_socket;
  struct sockaddr_in server_address;

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Error creating socket");
    return 1;
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
    perror("Invalid address/ Address not supported");
    close(server_socket);
    return 1;
  }

  if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
    perror("Error connecting to server");
    close(server_socket);
    return 1;
  }
  printf("Connected to server at %s:%d\n", SERVER_IP, PORT);

  // Create new empty node struct or import backup from .bin if available
  const char *dirpath = ".";
  Node *root;

  FILE *file = fopen("node_data.bin", "rb");
  if (!file) {
    printf("No saved directory tree, creating new.\n");
    root = create_node(dirpath, FOLDER_NODE);
  } else {
    root = load_node(file);
    fclose(file);
  }

  // compare root(node) w/ local file changes, and upload to server through server_socket.
  // the core of the backup logic.
  processTree(dirpath, root, server_socket);

  // tell server we've finished sending data 
  size_t filename_size = 0;
  send(server_socket, &filename_size, sizeof(filename_size), 0);

  printf("Tree Structure:\n");
  print_tree(root, 0);

  // Store Node for future use
  file = fopen("node_data.bin", "wb");
  if (!file) {
    perror("Failed to open file for writing");
    close(server_socket);
    return 1;
  }
  save_node(file, root);
  fclose(file);

  close(server_socket);
  free_tree(root);
  return 0;
}
