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
#define BUFFER_SIZE 1024

int main(){
    int serverFd, newSocket;
    struct sockaddr_in address;
    int addrLen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char message[BUFFER_SIZE] = {0};    //for server outgoing
    int valread;

    //Creating socket with IPv4, TCP, and IP protocol
    if((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        fprintf(stderr, "socket failed error: %d (%s)\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    //socket option to reuse address if port is in TIME_WAIT state
    int opt = 1;
    if(setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsocket error");
        close(serverFd);
        exit(EXIT_FAILURE);
    }

    //prepare sockaddr_in struct
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //binding to address and port
    if(bind(serverFd, (struct sockaddr *)&address, addrLen) == -1){
        fprintf(stderr, "bind failed with error: %d (%s)\n", errno, strerror(errno));
        close(serverFd);
        exit(EXIT_FAILURE);
    }

    //listen to incoming connection
    if(listen(serverFd, 3) < 0){
        fprintf(stderr, "listen failed with error: %d (%s)\n", errno, strerror(errno));
        close(serverFd);
        exit(EXIT_FAILURE);
    }

    printf("server listening on port: %d\n", PORT);

    //accept incoming connection
    if((newSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t *)&addrLen)) == -1){
        fprintf(stderr, "accept failed with error: %d (%s)\n", errno, strerror(errno));
        close(serverFd);
        exit(EXIT_FAILURE);
    }

    printf("Connection accepted from: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    printf("Type 'quit' or 'exit' to end connection\n");

    while (1) {
        //read data from client
        valread = recv(newSocket, buffer, BUFFER_SIZE - 1, 0);

        if(valread > 0){
            buffer[valread] = '\0';
            printf("Received Message: %s\n", buffer);

            //check for client exit
            if(strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0){
                printf("Client requested to end chat. Closing connection");
                break;
            }
        }
        else if (valread == 0) {
            printf("Client disconnected");
            break;
        }
        else{
            fprintf(stderr, "recv failed with error: %d (%s)\n", errno, strerror(errno));
            break;
        }

        //send message to client from server
        printf("Server: ");
        if(fgets(message, BUFFER_SIZE, stdin) == NULL){
            fprintf(stderr, "Error reading input.\n");
            break;
        }
        message[strcspn(message, "\n")] = 0;    //remove trailing new line

        //check server exit
        if(strcmp(message, "quit") == 0 || strcmp(message, "exit") == 0){
            printf("Server requested to end the connection. Closing connection.\n");
            send(newSocket, message, strlen(message), 0);   //send to client
            break;
        }

        //send message
        if(send(newSocket, message, strlen(message), 0) < 0){
            fprintf(stderr, "Send failed with error: %d (%s)\n", errno, strerror(errno));
            break;
        }
    }

    //close connected socket
    close(newSocket);
    printf("Server socket closed\n");

    //close listening socket
    close(serverFd);
    printf("Server listening socket closed\n");

    return 0;
}
