#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define MAX_ATTEMPTS 3
#define MAX_BUFFER 1024

typedef struct {
    char username[50];
    char password[50];
    int isBlocked;
    char homepage[100];
} Account;

// Kiem tra thong tin dang nhap
int check_credentials(char *username, char *password, Account *accounts, int num_accounts) {
    for (int i = 0; i < num_accounts; i++) {
        if (strcmp(username, accounts[i].username) == 0) {
            if (accounts[i].isBlocked == 0) {
                return -2;  // tai khoan bi khoa
            }
            if (strcmp(password, accounts[i].password) == 0) {
                return 1;  // mk dung
            } else {
                return 0;  // mk sai
            }
        }
    }
    return -1;  // khong tim thay tai khoan
}


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


void save_all_accounts(const char *filename, Account *accounts, int num_accounts) {
    FILE *file = fopen(filename, "w");  // M? file d? ghi l?i toàn b?
    if (!file) {
        perror("Error opening file to write");
        return;
    }

    for (int i = 0; i < num_accounts; i++) {
        fprintf(file, "%s %s %d %s\n", accounts[i].username, accounts[i].password, accounts[i].isBlocked, accounts[i].homepage);
    }

    fclose(file);
}


int validate_password(char *password) {
    for (int i = 0; i < strlen(password); i++) {
        if (!isalnum(password[i])) {
            return 0;  // Phat hien ki tu khong hop le
        }
    }
    return 1;  // tat ca ki tu hop le
}


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

    //tao socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    printf("Server is running on port %d...\n", port);

    socklen_t len = sizeof(client_addr);
    int n;
    char username[50], password[50];
    int logged_in = 0;
    int current_account_index = -1;

    while (1) {
        if (!logged_in) {
            // Nhan yeu cau dang nhap
            n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, (struct sockaddr *)&client_addr, &len);
            buffer[n] = '\0';
            sscanf(buffer, "%s %s", username, password);

            int result = check_credentials(username, password, accounts, num_accounts);
            if (result == 1) {
                sendto(sockfd, "OK", strlen("OK"), 0, (const struct sockaddr *)&client_addr, len);
                printf("User %s logged in successfully.\n", username);
                logged_in = 1;
                for (int i = 0; i < num_accounts; i++) {
                    if (strcmp(accounts[i].username, username) == 0) {
                        current_account_index = i;
                        break;
                    }
                }
            } else if (result == 0) {
                attempt_count++;
                if (attempt_count >= MAX_ATTEMPTS) {
                    accounts[current_account_index].isBlocked = 0;  // Khoa tai khoan
                    save_all_accounts("nguoidung.txt", accounts, num_accounts); // Cap nhat file
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
        } else {
            // Nhan yeu cau sau dang nhap
            n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, (struct sockaddr *)&client_addr, &len);
            buffer[n] = '\0';

            if (strcmp(buffer, "bye") == 0) {
                sendto(sockfd, "Goodbye", strlen("Goodbye"), 0, (const struct sockaddr *)&client_addr, len);
                printf("User %s signed out.\n", username);
                logged_in = 0;
                current_account_index = -1;
            } else if (strcmp(buffer, "homepage") == 0) {
                sendto(sockfd, accounts[current_account_index].homepage, strlen(accounts[current_account_index].homepage), 0, (const struct sockaddr *)&client_addr, len);
            } else if (strcmp(buffer, "change_password") == 0) {
                // Nhan mat khau tu client
                n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, (struct sockaddr *)&client_addr, &len);
                buffer[n] = '\0';

                // Kiem tra tinh hop le cua mat khau
                if (validate_password(buffer)) {
                    char letters[MAX_BUFFER], digits[MAX_BUFFER];
                    split_password(buffer, letters, digits);

                    // Cap nhat mk
                    strcpy(accounts[current_account_index].password, buffer);
                    save_all_accounts("nguoidung.txt", accounts, num_accounts); 

                    // Gui lai chuoi chu cai va chu so
                    sendto(sockfd, letters, strlen(letters), 0, (const struct sockaddr *)&client_addr, len);
                    sendto(sockfd, digits, strlen(digits), 0, (const struct sockaddr *)&client_addr, len);
                } else {
                    // Mat khau khong hop le
                    sendto(sockfd, "Error: Invalid password", strlen("Error: Invalid password"), 0, (const struct sockaddr *)&client_addr, len);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}

