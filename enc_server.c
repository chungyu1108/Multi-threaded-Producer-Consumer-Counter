#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, int portNumber){
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

// Check if the client is the appropriate counterpart
int checkClient(int connectionSocket){
  int charsRead;
  char buffer[256];
  
  // Receive message from the client
  charsRead = recv(connectionSocket, buffer, 255, 0); 
  if (charsRead < 0){
    error("ERROR reading from socket");
    return 0;
  }
  
  // Check if the message is from the correct client
  if(strcmp(buffer, "enc") != 0){
    charsRead = send(connectionSocket, "no", 2, 0);
    printf("Received message from the wrong client\n");
    return 0;
  }
  else{
    charsRead = send(connectionSocket, "yes", 3, 0);
    // printf("SERVER: Right client \"%s\"\n", buffer);
  }
  
  return 1;
}

// Encrypt the plaintext using the key
void encrypt(char* plaintext, char* key) {
  int i;
  for (i = 0; i < strlen(plaintext); i++) {
    if (plaintext[i] == ' ')
      plaintext[i] = 26 + 'A';
    if (key[i] == ' ')
      key[i] = 26 + 'A';
    plaintext[i] = ((plaintext[i] - 'A') + (key[i] - 'A')) % 27 + 'A';
    if (plaintext[i] == 26 + 'A')
      plaintext[i] = ' ';
  }
}

// Perform encryption and send the ciphertext back to the client
void performEncryption(int connectionSocket){
  int charsWritten, charsRead, plaintextSize;
  char buffer[256];
  char* plaintext;
  char* key;
  char* getSizeMsg = "get size";
	char* getTextMsg = "get text";

  // Receive plaintext size from the client
  charsRead = recv(connectionSocket, buffer, 256, 0); 
  if (charsRead < 0) error("SERVER: ERROR reading plaintext size from socket");

  plaintextSize = atoi(buffer) + 1;
  plaintext = (char *)malloc((plaintextSize) * sizeof(char));
  memset(plaintext, '\0', plaintextSize);
  key = (char *)malloc((plaintextSize) * sizeof(char));
  memset(key, '\0', plaintextSize);

  // Send message to indicate readiness to receive plaintext
  charsWritten = 0;
  while(charsWritten <= strlen(getSizeMsg)){
    charsWritten += send(connectionSocket, getSizeMsg, strlen(getSizeMsg), 0);
    if (charsWritten < 0) error("CLIENT: ERROR writing plaintext size message to socket");
  }

  // Receive plaintext from the client
  charsRead = 0;
  while(charsRead < plaintextSize - 1){
    charsRead = recv(connectionSocket, plaintext, plaintextSize - 1, 0); 
    if (charsRead < 0) error("SERVER: ERROR reading plaintext from socket");
    // printf("plaintext: %s\n", plaintext);
  }

  // Receive key from the client
  charsRead = 0;
  while(charsRead < plaintextSize - 1 || (strcmp(key, plaintext) == 0)){
    charsRead = recv(connectionSocket, key, plaintextSize - 1, 0); 
    if (charsRead < 0) error("SERVER: ERROR reading key from socket");
  }
  // printf("key: %s\n", key);

  // Perform encryption
  encrypt(plaintext, key);
  // printf("ciphertext before send: %s", plaintext);

  // Send the ciphertext back to the client
  charsWritten = 0;
  while(charsWritten <= plaintextSize - 1){
    charsWritten += send(connectionSocket, plaintext, plaintextSize - 1, 0);
    if (charsWritten < 0) error("CLIENT: ERROR writing ciphertext message to socket");
  }

  // Free allocated memory
  free(plaintext);
  free(key);
}

int main(int argc, char *argv[]){
  int connectionSocket, charsRead;
  char buffer[256];
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connections. Allow up to 5 connections to queue up
  listen(listenSocket, 5);
  
  pid_t parentPID = getpid();
  int numChildSockets = 0;
	int connectedChildSocketFD;
	int numChildProcs = 0;
	int childProcs[5];

  // Accept a connection, blocking if one is not available until one connects
  while(parentPID == getpid()){
    // Check if any child processes have finished
    if (numChildProcs > 0) {
      int i;
			for (i = 0; i < numChildProcs; i++) {
				int curChildStatus;
				pid_t curChild = waitpid(childProcs[i], &curChildStatus, WNOHANG);
				if (curChild != 0) {
					// Print information about the child process
					char errMsg[1000];
					if (WIFEXITED(curChildStatus) != 0) {
						sprintf(errMsg, "SERVER: Child pid %d is done: exit value %d\n", curChild, WEXITSTATUS(curChildStatus));
						// perror(errMsg);
					}
					if (WIFSIGNALED(curChildStatus) != 0) {
						sprintf(errMsg, "SERVER: Child pid %d is done: terminated by signal %d\n", curChild, WTERMSIG(curChildStatus));
						// perror(errMsg);
					}

					// Replace the child process with the most recent child
					childProcs[i] = childProcs[numChildProcs - 1];
					numChildProcs--;
					numChildSockets--;
				}
			}
		}

		// Check if the maximum number of child sockets has been reached
		if (numChildSockets < 5) {
      sizeOfClientInfo = sizeof(clientAddress);
      // Accept the connection request and generate the socket used for communication
      connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
      if (connectionSocket < 0){
        error("ERROR on accept");
      }

      // Fork a child process to handle the encryption for this client connection
      pid_t childPid = fork();
      switch (childPid){
        case -1:{
          error("fork() failed!\n");
          exit(1);
        }
        case 0:{// Child process
          if(checkClient(connectionSocket) == 1){
            performEncryption(connectionSocket);
          }
          break;
        }
        default:
          numChildSockets++;
          childProcs[numChildProcs] = childPid;
          numChildProcs++;
          break;
      }

      close(connectionSocket); 
    }
  }

  // Close the listening socket
  close(listenSocket); 
  return 0;
}
