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
    const char *message = "Hello From client";

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

    //send message to server
    send(sock, message, strlen(message), 0);
    printf("Message sent: %s\n", message);

    //read echo from server
    int valread = recv(sock, buffer, BUFFER_SIZE - 1, 0); //-1 for null termination and spaces

    if(valread > 0){
        buffer[valread] = '\0'; //Null termi received data
        printf("Received echo: %s\n", buffer);
    }
    else if (valread == 0) {
        printf("Server Closed connection.\n");
    }
    else{
        fprintf(stderr, "recv failed with error: %d (%s)\n", errno, strerror(errno));
    }

    //close socket
    close(sock);
    printf("Client socket closed");

    return 0;
}
