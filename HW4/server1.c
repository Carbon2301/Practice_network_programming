#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_ATTEMPTS 3
#define MAX_BUFFER 1024

typedef struct {
    char username[50];
    char password[50];
    int isBlocked;
} Account;

// Function to check login credentials
int check_credentials(char *username, char *password, Account *accounts, int num_accounts) {
    for (int i = 0; i < num_accounts; i++) {
        if (strcmp(username, accounts[i].username) == 0) {
            if (accounts[i].isBlocked) {
                return -2;  // Account is blocked
            }
            if (strcmp(password, accounts[i].password) == 0) {
                return 1;  // Correct password
            } else {
                return 0;  // Incorrect password
            }
        }
    }
    return -1;  // Username not found
}

// Load user accounts from file
int load_accounts(const char *filename, Account *accounts) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }
    
    int i = 0;
    while (fscanf(file, "%s %s %d", accounts[i].username, accounts[i].password, &accounts[i].isBlocked) != EOF) {
        i++;
    }

    fclose(file);
    return i;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <PortNumber>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[MAX_BUFFER];
    Account accounts[100];
    int num_accounts = load_accounts("nguoidung.txt", accounts);
    int attempt_count = 0;

    if (num_accounts < 0) {
        return 1;
    }

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket to address
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    printf("Server is running on port %d...\n", port);

    socklen_t len = sizeof(client_addr);
    int n;

    while (1) {
        // Receive login request
        n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, (struct sockaddr *)&client_addr, &len);
        buffer[n] = '\0';
        char username[50], password[50];
        sscanf(buffer, "%s %s", username, password);

        int result = check_credentials(username, password, accounts, num_accounts);
        if (result == 1) {
            sendto(sockfd, "OK", strlen("OK"), 0, (const struct sockaddr *)&client_addr, len);
            printf("User %s logged in successfully.\n", username);
        } else if (result == 0) {
            attempt_count++;
            if (attempt_count >= MAX_ATTEMPTS) {
                for (int i = 0; i < num_accounts; i++) {
                    if (strcmp(accounts[i].username, username) == 0) {
                        accounts[i].isBlocked = 1;
                        break;
                    }
                }
                sendto(sockfd, "Account is blocked", strlen("Account is blocked"), 0, (const struct sockaddr *)&client_addr, len);
                attempt_count = 0;
            } else {
                sendto(sockfd, "Not OK", strlen("Not OK"), 0, (const struct sockaddr *)&client_addr, len);
            }
        } else if (result == -2) {
            sendto(sockfd, "Account not ready", strlen("Account not ready"), 0, (const struct sockaddr *)&client_addr, len);
        } else {
            sendto(sockfd, "User not found", strlen("User not found"), 0, (const struct sockaddr *)&client_addr, len);
        }
    }

    close(sockfd);
    return 0;
}

