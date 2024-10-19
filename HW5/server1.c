#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define MAX_BUFFER 1024
#define MAX_ATTEMPTS 3

typedef struct {
    char username[50];
    char password[50];
    int isBlocked;
    char homepage[100];
} Account;

// Load accounts from file
int load_accounts(const char *filename, Account *accounts) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    int i = 0;
    while (fscanf(file, "%s %s %d %s", accounts[i].username, accounts[i].password, &accounts[i].isBlocked, accounts[i].homepage) != EOF) {
        i++;
    }

    fclose(file);
    return i;
}

// Save accounts to file
void save_all_accounts(const char *filename, Account *accounts, int num_accounts) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file to write");
        return;
    }

    for (int i = 0; i < num_accounts; i++) {
        fprintf(file, "%s %s %d %s\n", accounts[i].username, accounts[i].password, accounts[i].isBlocked, accounts[i].homepage);
    }

    fclose(file);
}

// Validate password
int validate_password(char *password) {
    for (int i = 0; i < strlen(password); i++) {
        if (!isalnum(password[i])) {
            return 0;  // Invalid character found
        }
    }
    return 1;  // All characters valid
}

// Split password into letters and digits
void split_password(char *password, char *letters, char *digits) {
    int l_index = 0, d_index = 0;
    for (int i = 0; i < strlen(password); i++) {
        if (isalpha(password[i])) {
            letters[l_index++] = password[i];
        } else if (isdigit(password[i])) {
            digits[d_index++] = password[i];
        }
    }
    letters[l_index] = '\0';
    digits[d_index] = '\0';
}

// Handle client requests
void handle_client(int new_sock, Account *accounts, int num_accounts) {
    char buffer[MAX_BUFFER];
    int attempt_count = 0;
    int logged_in = 0;
    int current_account_index = -1;
    char username[50], password[50];

    while (1) {
        // Receive username and password from client
        recv(new_sock, buffer, MAX_BUFFER, 0);
        sscanf(buffer, "%s %s", username, password);

        // Check credentials
        for (int i = 0; i < num_accounts; i++) {
            if (strcmp(username, accounts[i].username) == 0) {
                if (accounts[i].isBlocked == 0) {
                    send(new_sock, "account not ready", strlen("account not ready"), 0);
                    return;
                }
                if (strcmp(password, accounts[i].password) == 0) {
                    send(new_sock, "OK", strlen("OK"), 0);
                    logged_in = 1;
                    current_account_index = i;
                    break;
                } else {
                    attempt_count++;
                    if (attempt_count >= MAX_ATTEMPTS) {
                        accounts[i].isBlocked = 0;  // Block account
                        save_all_accounts("nguoidung.txt", accounts, num_accounts);
                        send(new_sock, "Account is blocked", strlen("Account is blocked"), 0);
                        return;
                    }
                    send(new_sock, "not OK", strlen("not OK"), 0);
                }
            }
        }

        if (!logged_in) {
            send(new_sock, "User not found", strlen("User not found"), 0);
            return;
        }

        // After login, handle commands
        while (logged_in) {
            recv(new_sock, buffer, MAX_BUFFER, 0);
            if (strcmp(buffer, "bye") == 0) {
                send(new_sock, "Goodbye", strlen("Goodbye"), 0);
                logged_in = 0;
                break;
            } else if (strcmp(buffer, "homepage") == 0) {
                send(new_sock, accounts[current_account_index].homepage, strlen(accounts[current_account_index].homepage), 0);
            } else if (strcmp(buffer, "change_password") == 0) {
                // Receive new password
                recv(new_sock, buffer, MAX_BUFFER, 0);
                if (!validate_password(buffer)) {
                    send(new_sock, "Error: Invalid password", strlen("Error: Invalid password"), 0);
                } else {
                    char letters[MAX_BUFFER], digits[MAX_BUFFER];
                    split_password(buffer, letters, digits);
                    strcpy(accounts[current_account_index].password, buffer);
                    save_all_accounts("nguoidung.txt", accounts, num_accounts);
                    send(new_sock, letters, strlen(letters), 0);
                    send(new_sock, digits, strlen(digits), 0);
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <PortNumber>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int server_sock, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    Account accounts[100];
    int num_accounts = load_accounts("nguoidung.txt", accounts);

    if (num_accounts < 0) {
        return 1;
    }

    // Create TCP socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        close(server_sock);
        return 1;
    }

    printf("Server is running on port %d...\n", port);

    while (1) {
        // Accept new connection
        new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (new_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // Handle client in a new process/thread
        handle_client(new_sock, accounts, num_accounts);

        close(new_sock);
    }

    close(server_sock);
    return 0;
}

