#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IPAddress> <PortNumber>\n", argv[0]);
        return 1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[MAX_BUFFER];
    char username[50], password[50];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Fill server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    int n;

    while (1) {
        // Input username and password
        printf("Enter username: ");
        scanf("%s", username);
        printf("Enter password: ");
        scanf("%s", password);

        // Send username and password to server
        snprintf(buffer, sizeof(buffer), "%s %s", username, password);
        sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));

        // Receive response from server
        n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, NULL, NULL);
        buffer[n] = '\0';
        printf("Server: %s\n", buffer);

        if (strcmp(buffer, "OK") == 0) {
            printf("Login successful!\n");
            break;
        } else if (strcmp(buffer, "Account is blocked") == 0) {
            printf("Account has been blocked after multiple failed attempts.\n");
            break;
        } else if (strcmp(buffer, "Not OK") == 0) {
            printf("Incorrect password. Try again.\n");
        } else if (strcmp(buffer, "User not found") == 0) {
            printf("Username does not exist.\n");
        } else if (strcmp(buffer, "Account not ready") == 0) {
            printf("Account is blocked. Contact admin.\n");
            break;
        }
    }

    close(sockfd);
    return 0;
}

