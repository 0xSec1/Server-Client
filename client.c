#include <asm-generic/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define PORT 6767
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main(){
    int sock = 0;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE] = {0};
    char message[BUFFER_SIZE] = {0};    //for client outgoing

    //create socket file descriptor
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        fprintf(stderr, "socket creation failed with error: %d (%s)\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    //convert IPv4 to binary form using inet_pton
    if(inet_pton(AF_INET, SERVER_IP, &serverAddress.sin_addr) <= 0){
        fprintf(stderr, "Invalid address/Address not supported\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    //connect to server
    if(connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1){
        fprintf(stderr, "Connection failed with error: %d (%s)", errno, strerror(errno));
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server %s:%d\n", SERVER_IP, PORT);
    printf("Type 'quit' or 'exit' to end connection\n");

    //main loop
    while(1){
        printf("Client: ");
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            fprintf(stderr, "Error reading input.\n");
            break;
        }
        message[strcspn(message, "\n")] = 0;     //remove trailing newline

        //check for client's exit
        if(strcmp(message, "quit") == 0 || strcmp(message, "exit") == 0){
            printf("Client requested to end connection. Closing connection.\n");
            send(sock, buffer, BUFFER_SIZE, 0);     //send quit request to server
            break;
        }

        //send message
        if(send(sock, message, strlen(message), 0) < 0){
            fprintf(stderr, "Send failed with error: %d (%s)\n", errno, strerror(errno));
            break;
        }

        //Receive from server
        int valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if(valread > 0){
            buffer[valread] = '\0'; //null terminate received data
            printf("Server: %s\n", buffer);

            //check for server exit
            if(strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0){
                printf("Server requested to end connection. Closing connection.\n");
                break;
            }
        }
    }

    //close socket
    close(sock);
    printf("Client socket closed");

    return 0;
}
