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

    //read data from client
    valread = recv(newSocket, buffer, BUFFER_SIZE - 1, 0);

    if(valread > 0){
        buffer[valread] = '\0';
        printf("Received Message: %s\n", buffer);
        //echo back to client
        send(newSocket, buffer, strlen(buffer), 0);
        printf("Echoed back: %s\n", buffer);
    }
    else if (valread == 0) {
        printf("Client disconnected");
    }
    else{
        fprintf(stderr, "recv failed with error: %d (%s)\n", errno, strerror(errno));
    }

    //close connected socket
    close(newSocket);
    printf("Server socket closed\n");

    //close listening socket
    close(serverFd);
    printf("Server listening socket closed\n");

    return 0;
}
