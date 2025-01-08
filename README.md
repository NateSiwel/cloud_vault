# CloudVault

Automated file backup solution. Select file directories on a client device to automatically backup to a local server.

## Startup command 
Backup command inside **client/** directory
- gcc -o out src/*.c -Iinclude -lcrypto

Run server inside **server/src** directory
- gcc main.c

## TODO 
- logic to ensure server has recieved all data from client before closing connection. Current solution is sleep(2) in client/src/main.c
- clean filepaths clientside that are sent to server 
