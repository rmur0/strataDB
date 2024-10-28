#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "master-main.h"
#define PORT 12345
void handle_client(int client_fd) {
    char buffer[1024];
    char command[4];
    char key[256];
    char value[256];
    ssize_t bytes_read;
    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        memset(command, 0, sizeof(command));
        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        sscanf(buffer, "%3s %255s %255s", command, key, value);
        if (strcmp(command, "SET") == 0) {
            if (set(key, value) == 0) {
                write(client_fd, "OK\n", 3);
            } else {
                write(client_fd, "ERROR: Failed to set value\n", 27);
            }
        } else if (strcmp(command, "GET") == 0) {
            char *result = get(key);
            write(client_fd, result, strlen(result));
            write(client_fd, "\n", 1);
            free(result);
        } else if (strcmp(command, "DEL") == 0) {
            if (set(key, tombstone) == 0) {
                write(client_fd, "DELETED\n", 8);
                printf("Deleted \n");
            } else {
                write(client_fd, "ERROR: Failed to delete key\n", 28);
            }
        } else {
            write(client_fd, "ERROR: Unknown command\n", 23);
        }
    }
    close(client_fd);
}
int main(){
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if (construct_hash_map_from_directory() != 0) {
        fprintf(stderr, "Failed to initialize the database.\n");
        exit(EXIT_FAILURE);
    }
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Failed to bind socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, SOMAXCONN) < 0){
        perror("Failed to listen on socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d\n", PORT);
    while (1){
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0){
            perror("Failed to accept client connection");
            continue;
        }
        handle_client(client_fd);
    }
    close(server_fd);
    return 0;
}

