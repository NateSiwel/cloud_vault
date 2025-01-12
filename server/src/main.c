#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080
#define MAX_CLIENTS 1  
#define BUFFER_SIZE 1024 
#define BACKUP_DIR "backup"

/* 
 * Server logic to accept connection and data from client
 */

int main() {
  int server_socket, client_socket;
  struct sockaddr_in server_address, client_address;
  socklen_t client_address_len = sizeof(client_address);
  ssize_t bytes_received;
  size_t filename_size;
  size_t file_size;
  char *filename = NULL;
  char *file_content = NULL;
  char filepath[256];
  int processing_status;

  // Create the backup directory if it doesn't exist in the current directory
  struct stat st = {0};
  if (stat(BACKUP_DIR, &st) == -1) {
    if (mkdir(BACKUP_DIR, 0777) == -1) {
      perror("Error creating backup directory");
      return 1;
    } else {
      printf("Created backup directory: %s\n", BACKUP_DIR);
    }
  }

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Error creating socket");
    return 1;
  }

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(PORT); 

  // bind socket to address
  if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
    perror("Error binding socket");
    close(server_socket);
    return 1;
  }

  // listen for connection
  if (listen(server_socket, MAX_CLIENTS) == -1) {
    perror("Error listening for connections");
    close(server_socket);
    return 1;
  }

  printf("Server listening on port %d...\n", PORT);

  // accept incoming connections
  while (1) {
    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
    if (client_socket == -1) {
      perror("Error accepting connection");
      continue; // no client yet - restart loop
    }

    printf("Connection from: %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    while (1) { // Handling client in a loop until disconnection

      // Receive filename size - disconnect if 0 
      bytes_received = recv(client_socket, &filename_size, sizeof(filename_size), 0);
      if (bytes_received <= 0) {
	if (bytes_received == 0) {
	  printf("Client disconnected: %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
	} else {
	  perror("Error receiving filename size");
	}
	break;
      }

      // Allocate space for filename, +1 for null terminator
      filename = (char*)malloc(filename_size + 1);
      if (filename == NULL) {
	perror("Error allocating memory for filename");
	break;
      }

      // Receive filename - disconnect if 0 
      bytes_received = recv(client_socket, filename, filename_size, 0);
      if (bytes_received <= 0) {
	if (bytes_received == 0) {
	  printf("Client disconnected: %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
	} else {
	  perror("Error receiving filename");
	}
	free(filename);
	break; 
      }
      filename[filename_size] = '\0';
      printf("Filename: %s\n", filename);

      // if filename ends with '/', type is folder. mkdir here, then restart loop. 
      // This logic works, but is dependent on recieving folders from the client before the files that are within them.
      // Shouldn't be a problem with current client code.
      if (filename[filename_size - 1] == '/') {
	snprintf(filepath, sizeof(filepath), "%s/%s", BACKUP_DIR, filename);
	if (mkdir(filepath, 0777) == -1) {
	  perror("Error creating directory");
	  processing_status = -1;
	} else {
	  printf("Created directory: %s\n", filepath);
	  processing_status = 1; 
	}

	send(client_socket, &processing_status, sizeof(processing_status), 0);

	free(filename);
	filename = NULL;
	continue;
      }

      // Receive file size - disconnect if 0 
      bytes_received = recv(client_socket, &file_size, sizeof(file_size), 0);
      if (bytes_received <= 0) {
	if (bytes_received == 0) {
	  printf("Client disconnected: %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
	} else {
	  perror("Error receiving file size");
	}
	free(filename);
	break; 
      }

      //Allocate space for file content
      file_content = (char *)malloc(file_size);
      if (file_content == NULL) {
	perror("Error allocating memory for file content");
	free(filename);
	break;
      }

      // Receive file content - disconnect if 0 bytes rec
      bytes_received = recv(client_socket, file_content, file_size, 0);
      if (bytes_received <= 0)
      {
	if (bytes_received == 0)
	{
	  printf("Client disconnected: %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
	} else {
	  perror("Error receiving file content");
	}
	free(filename);
	free(file_content);
	break;
      }

      snprintf(filepath, sizeof(filepath), "%s/%s", BACKUP_DIR, filename);

      // Reconstruct the file and store it on disk
      FILE *fp = fopen(filepath, "wb");
      if (fp == NULL) {
	perror("Error opening file for writing");
	processing_status = -1; 
      } else {
	size_t bytes_written = fwrite(file_content, 1, file_size, fp);
	if (bytes_written < file_size) {
	  perror("Error writing to file");
	  processing_status = -1; 
	} else {
	  printf("File '%s' saved successfully to '%s' (%zu bytes).\n", filename, filepath, bytes_written);
	  processing_status = 1;
	}
	fclose(fp);
      }

      send(client_socket, &processing_status, sizeof(processing_status),0);

      free(filename);
      free(file_content);
      filename = NULL;
      file_content = NULL;
    }

    close(client_socket);
    printf("Waiting for the next client\n");
  }


  close(server_socket); // will never be reached
  return 0;
}
