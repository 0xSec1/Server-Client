#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 6767
#define MAX_CLIENTS 5   //maximum number of concurrent clients
#define BUFFER_SIZE 1024

int main(){
    int main_socket, newSocket, activity, valread, i;
    int client_sockets[MAX_CLIENTS];    //array to hold client socket descriptors
    int max_sd; //max fd number

    struct sockaddr_in address;
    int addrLen = sizeof(address);
    char buffer[BUFFER_SIZE];

    //set socket descriptor for select()
    fd_set readfds;

    //Initialise all client_socket to 0
    for(int i = 0; i < MAX_CLIENTS; i++){
        client_sockets[i] = 0;
    }

    //Creating socket with IPv4, TCP, and IP protocol
    if((main_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        fprintf(stderr, "socket failed error: %d (%s)\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    //socket option to reuse address if port is in TIME_WAIT state
    int opt = 1;
    if(setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsocket error");
        close(main_socket);
        exit(EXIT_FAILURE);
    }

    //prepare sockaddr_in struct
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //binding to address and port
    if(bind(main_socket, (struct sockaddr *)&address, addrLen) == -1){
        fprintf(stderr, "bind failed with error: %d (%s)\n", errno, strerror(errno));
        close(main_socket);
        exit(EXIT_FAILURE);
    }

    //listen to incoming connection
    if(listen(main_socket, 3) < 0){
        fprintf(stderr, "listen failed with error: %d (%s)\n", errno, strerror(errno));
        close(main_socket);
        exit(EXIT_FAILURE);
    }

    printf("server listening on port: %d\n", PORT);
    printf("Waiting for connections....\n");

    while (1) {
        //clear socket set
        FD_ZERO(&readfds);

        //add main socket to set
        FD_SET(main_socket, &readfds);
        max_sd = main_socket;

        //add child socket to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            //if valid socket descriptor exist, add to list
            if(sd > 0){
                FD_SET(sd, &readfds);
                if(sd > max_sd) max_sd = sd;
            }
        }

        //wait for activity on one of socket
        //TIMEOUT is NULL so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if(activity == -1){
            if(errno == EINTR) continue;
            fprintf(stderr, "select error: %d (%s)\n", errno, strerror(errno));
            break;      //try to recover or exit for now exit
        }

        //if something happen on main socket then its incoming request
        if(FD_ISSET(main_socket, &readfds)){
            if ((newSocket = accept(main_socket, (struct sockaddr *)&address, (socklen_t *)&addrLen)) == -1) {
                fprintf(stderr, "accept failed error: %d (%s)\n", errno, strerror(errno));
                break;
            }

            printf("New Connection, socket fd is %d, IP: %s, PORT: %d\n", newSocket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            //Add new socket to list of socket
            int added = 0;
            for(int i = 0; i < MAX_CLIENTS; i++){
                if(client_sockets[i] == 0){
                    client_sockets[i] = newSocket;
                    printf("Adding to list of socket as: %d\n", i);
                    added = 1;
                    break;
                }
            }

            //No space for clients
            if(!added){
                printf("Max clients reached. Connection refused.\n");
                const char *refuse_msg = "Server full try again later.\n";
                send(newSocket, refuse_msg, strlen(refuse_msg), 0);
                close(newSocket);
            }
        }

        //I/O operation on existing socket
        for(i = 0; i < MAX_CLIENTS; i++){
            int sd = client_sockets[i];
            if(sd == 0) continue;   //skip empty

            if(FD_ISSET(sd, &readfds)){
                memset(buffer, 0, BUFFER_SIZE);

                //Read incoming message
                valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);

                if(valread == 0){
                    //client disconnected
                    char client_ip[INET_ADDRSTRLEN];

                    //Get client info before closing socket
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrLen);
                    inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
                    printf("Client Disconnected, IP: %s, PORT: %d\n", client_ip, ntohs(address.sin_port));

                    //close socket and free slot
                    close(sd);
                    client_sockets[i] = 0;
                }
                else if (valread == -1){
                    fprintf(stderr, "recv failed on socket %d with error: %d (%s)\n", sd, errno, strerror(errno));
                    close(sd);
                    client_sockets[i] = 0;
                }
                else{
                    //Message received
                    buffer[valread] = '\0';
                    char client_ip[INET_ADDRSTRLEN];
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrLen);
                    inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);

                    printf("Message received from client %s:%d (socket %d): %s\n", client_ip, ntohs(address.sin_port), sd, buffer);

                    //check server side exit
                    if(strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0){
                        printf("Received server shutdown from client %d, Shutting down...", sd);
                        //send server shutdown message to all client
                        const char *shutdown_msg = "Server is shutting down.\n";
                        for(int j = 0; j < MAX_CLIENTS; j++){
                            if(client_sockets[j] != 0 && client_sockets[j] != sd){
                                send(client_sockets[j], shutdown_msg, strlen(shutdown_msg), 0);
                            }
                        }

                        close(sd);
                        client_sockets[i] = 0;
                        goto exit;  //jmp to exit
                    }

                    //Broadcast msg to all other client connected
                    for(int j = 0; j < MAX_CLIENTS; j++){
                        int dest_sd = client_sockets[j];
                        //send to valid socket not the sender
                        if(dest_sd != 0 && dest_sd != sd){
                            char broadcast_msg[BUFFER_SIZE + 50];   //increase buffer for prefix
                            snprintf(broadcast_msg, sizeof(broadcast_msg), "Client %d: %s", sd, buffer);

                            if(send(dest_sd, broadcast_msg, strlen(broadcast_msg), 0) < 0){
                                fprintf(stderr, "send to client %d failed with error: %d (%s)\n", dest_sd, errno, strerror(errno));
                            }
                        }
                    }
                }
            }
        }
    }

exit:
    //close all socket
    for(i = 0; i < MAX_CLIENTS; i++){
        if(client_sockets[i] != 0){
            close(client_sockets[i]);
        }
    }

    close(main_socket);
    printf("Server shutdown\n");
    return 0;
}
