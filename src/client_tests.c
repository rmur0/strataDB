#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#define SERVER_IP "127.0.0.1"
#define PORT 12345
int connect_to_server() {
    int client_fd;
    struct sockaddr_in server_addr;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    return client_fd;
}

void send_command(int client_fd, const char *command, const char *expected_response) {
    char buffer[1024];
    ssize_t bytes_read;

    send(client_fd, command, strlen(command), 0);
    bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        /*
        printf("Sent: %s\nReceived: %s\n", command, buffer);
        */
        if (expected_response && strcmp(buffer, expected_response) != 0) {
            printf("Test failed. Expected: %s, but got: %s\n", expected_response, buffer);
        }
    } else {
        printf("Error reading response: %s\n", strerror(errno));
    }
}


void test1(int client_fd) {
    printf("Starting test 1\n");
    send_command(client_fd, "SET Shea 2013\n", "OK\n");
    send_command(client_fd, "SET Shea 2014\n", "OK\n");
    send_command(client_fd, "GET Shea\n", "2014\n");
}

void test2(int client_fd) {
    printf("Starting test 2\n");
    send_command(client_fd, "SET Shea 2013\n", "OK\n");
    send_command(client_fd, "SET Shea 2014\n", "OK\n");
    send_command(client_fd, "SET Shea 2015\n", "OK\n");
    send_command(client_fd, "GET Shea\n", "2015\n");
}

void test3(int client_fd) {
    printf("Starting test 3\n");
    for (int i = 1834; i < 2025; i++) {
        char command[50];
        snprintf(command, sizeof(command), "SET Shea %d\n", i);
        send_command(client_fd, command, "OK\n");
    }
    send_command(client_fd, "GET Shea\n", "2024\n");
}

void test4(int client_fd) {
    printf("Starting test 4\n");
    send_command(client_fd, "SET Shea 1834\n", "OK\n");
    for (int i = 1835; i < 2025; i++) {
        char command[50];
        snprintf(command, sizeof(command), "SET Shea %d\n", i);
        send_command(client_fd, command, "OK\n");
    }
    send_command(client_fd, "GET Shea\n", "2024\n");
}



void test5(int client_fd) {
    printf("Starting test 5\n");
    clock_t start = clock();
    for (int i = 0; i < 15000; i++) {
        char command[50];
        snprintf(command, sizeof(command), "SET Shea %d\n", i);
        send_command(client_fd, command, "OK\n");
    }
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("High volume writes completed in %f seconds\n", time_taken);
    send_command(client_fd, "GET Shea\n", "14999\n");
}

void test6(int client_fd) {
    printf("Starting test 6\n");
    clock_t start = clock();
    for (int i = 0; i < 30000; i++) {
        char command[50];
        const char *key;
        if (i % 3 == 0) {
            key = "Shea";
        } else if (i % 3 == 1) {
            key = "Rogan";
        } else {
            key = "Shakespeare";
        }
        snprintf(command, sizeof(command), "SET %s %d\n", key, i);
        send_command(client_fd, command, "OK\n");
    }
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Alternating high volume writes completed in %f seconds\n", time_taken);
    send_command(client_fd, "GET Shea\n", "29997\n");
    send_command(client_fd, "GET Rogan\n", "29998\n");
    send_command(client_fd, "GET Shakespeare\n", "29999\n");
}


int main() {
    int client_fd = connect_to_server();
    /*
    test1(client_fd);
    test2(client_fd);
    test3(client_fd);
    test4(client_fd);
    
    test5(client_fd);
    */
    test6(client_fd);
    close(client_fd);
    return 0;
}
