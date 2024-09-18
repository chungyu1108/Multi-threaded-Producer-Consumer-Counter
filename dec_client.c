#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(1); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, int portNumber){
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname("localhost"); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

// Check if a file contains valid characters
int isValidFile(const char* fileName) {
  FILE* file = fopen(fileName, "r");
  if (file == NULL) { printf("fopen() failed\n"); return 0; } 
  
  int c;
  while ((c = fgetc(file)) != EOF) {
    if (c != ' ' && (c < 'A' || c > 'Z')) {
      if(c != '\n'){
        fclose(file);
        return 0;
      }
    }
  }

  fclose(file);
  return 1;
}

// Get the content of a file
char* getFileContent(const char* fileName) {
  FILE* file = fopen(fileName, "r");
  if (file == NULL)
    return NULL;

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  rewind(file);

  char* content = malloc(fileSize + 1);
  fread(content, fileSize, 1, file);
  content[fileSize - 1] = '\0';

  fclose(file);
  return content;
}

// Connect to the decryption server and request decryption
void requestDecryption(int socketFD, char* ciphertext, char* key){
  int charsWritten, charsRead;
  char buffer[256];
  memset(buffer, '\0', 256); 

  sprintf(buffer, "%d", strlen(ciphertext));
  // Send the ciphertext size to the server
  charsWritten = 0;
  while(charsWritten <= strlen(buffer)){
    charsWritten += send(socketFD, buffer, strlen(buffer), 0);
    if (charsWritten < 0) error("CLIENT: ERROR writing ciphertext size to socket");
  }
  
  memset(buffer, '\0', sizeof(buffer));
  
  // Receive acknowledgement from the server
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
	if (charsRead < 0) error("CLIENT: ERROR reading acknowledgement from socket");
  
  if (strcmp(buffer, "get size") == 0) {
    // Send the ciphertext to the server
    charsWritten = 0;
    while(charsWritten <= strlen(ciphertext)){
      charsWritten += send(socketFD, ciphertext, strlen(ciphertext), 0);
      if (charsWritten < 0) error("CLIENT: ERROR writing ciphertext to socket");
    }
	}
	else {
		error("CLIENT: Server failed to request ciphertext properly\n");
	}
  
  memset(buffer, '\0', sizeof(buffer));
  
  // Send the key to the server
  charsWritten = 0;
  while(charsWritten <= strlen(ciphertext)){
    charsWritten += send(socketFD, key, strlen(ciphertext), 0);
    if (charsWritten < 0) error("CLIENT: ERROR writing ciphertext to socket");
  }
}

int main(int argc, char *argv[]) {
  int socketFD, portNumber;
  struct sockaddr_in serverAddress;
  char buffer[256];
  
  // Check usage & args
  if (argc < 4) {
    fprintf(stderr,"USAGE: %s ciphertext key port\n", argv[0]);
    exit(1);
  }

  // Check if ciphertext file is valid
  if (!isValidFile(argv[1])) {
    fprintf(stderr, "CLIENT: ERROR: Invalid ciphertext file\n");
    exit(1);
  }

  // Check if key file is valid
  if (!isValidFile(argv[2])) {
    fprintf(stderr, "CLIENT: ERROR: Invalid key file\n");
    exit(1);
  }

  // Get ciphertext from file
  char* ciphertext = getFileContent(argv[1]);
  if (ciphertext == NULL) {
    fprintf(stderr, "CLIENT: ERROR: Failed to read ciphertext file\n");
    exit(1);
  }

  // Get key from file
  char* key = getFileContent(argv[2]);
  if (key == NULL) {
    fprintf(stderr, "CLIENT: ERROR: Failed to read key file\n");
    free(ciphertext);
    exit(1);
  }

  // Check if key length is shorter than ciphertext length
  if (strlen(key) < strlen(ciphertext)) {
    fprintf(stderr, "CLIENT: ERROR: Key length is shorter than ciphertext length\n");
    free(ciphertext);
    free(key);
    exit(1);
  }

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

  // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]));

  // Connect to the server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }
  
    // Receive the plaintext from the server
    memset(buffer, '\0', sizeof(buffer));
    int totalCharsRead = 0;
    int charsRead;
    while (totalCharsRead < strlen(ciphertext)) {
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
    if (charsRead < 0) {
        error("CLIENT: ERROR reading plaintext from socket");
    }
    totalCharsRead += charsRead;
    printf("%.*s", charsRead, buffer);
    memset(buffer, '\0', sizeof(buffer));
    }
    printf("\n");

  // Free allocated memory
  free(ciphertext);
  free(key);
  
  // Close the socket
  close(socketFD); 
  return 0;
}