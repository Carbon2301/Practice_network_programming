#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdbool.h>

#define BUFF_SIZE 255
#define MAX_CLIENTS 100
#define FILENAME "account.txt"

// Cấu trúc lưu thông tin tài khoản
typedef struct {
    char username[BUFF_SIZE];
    char password[BUFF_SIZE];
    int status; // 1: active, 0: blocked
} Account;

// Cấu trúc client
typedef struct {
    int fd;
    char username[BUFF_SIZE];
    int loginStatus; // 0: chưa đăng nhập, 1: nhập username, 2: đăng nhập thành công
    bool active;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
int clientCount = 0;

// Khởi tạo danh sách client
void initClients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].active = false;
    }
}

// Đọc danh sách tài khoản từ file
int loadAccounts(Account accounts[], int *count) {
    FILE *file = fopen(FILENAME, "r");
    if (!file) {
        perror("Error opening account file");
        return -1;
    }

    char line[BUFF_SIZE];
    *count = 0;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; // Xóa ký tự newline
        sscanf(line, "%s %s %d", accounts[*count].username, accounts[*count].password, &accounts[*count].status);
        (*count)++;
    }

    fclose(file);
    return 0;
}

// Tìm tài khoản trong danh sách
int findAccount(Account accounts[], int count, const char *username) {
    for (int i = 0; i < count; i++) {
        if (strcmp(accounts[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

// Cập nhật mật khẩu trong file và đồng bộ lại danh sách
int updatePassword(Account accounts[], int *count, const char *username, const char *newPassword) {
    FILE *file = fopen(FILENAME, "r+");
    if (!file) {
        perror("Error opening account file for update");
        return -1;
    }

    char lines[MAX_CLIENTS][BUFF_SIZE];
    int localCount = 0;
    char line[BUFF_SIZE];
    int updated = 0;

    // Đọc toàn bộ file vào bộ nhớ
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; // Xóa newline
        char uname[BUFF_SIZE], pass[BUFF_SIZE];
        int status;
        sscanf(line, "%s %s %d", uname, pass, &status);
        if (strcmp(uname, username) == 0) {
            snprintf(lines[localCount], sizeof(lines[localCount]), "%s %s %d", uname, newPassword, status);
            updated = 1;
        } else {
            strcpy(lines[localCount], line);
        }
        localCount++;
    }

    // Ghi lại toàn bộ file
    freopen(FILENAME, "w", file);
    for (int i = 0; i < localCount; i++) {
        fprintf(file, "%s\n", lines[i]);
    }

    fclose(file);

    if (updated) {
        // Cập nhật danh sách tài khoản trong bộ nhớ
        if (loadAccounts(accounts, count) < 0) {
            fprintf(stderr, "Error reloading accounts after update\n");
            return -1;
        }
    }

    return updated ? 0 : -1;
}

// Xử lý đầu vào từ client
void handleClientInput(int index, Account accounts[], int *accountCount, fd_set *masterSet) {
    char buff[BUFF_SIZE];
    int bytesRead = recv(clients[index].fd, buff, BUFF_SIZE, 0);

    if (bytesRead <= 0) {
        printf("Client %d disconnected\n", index);
        close(clients[index].fd);
        FD_CLR(clients[index].fd, masterSet);  // Loại bỏ client khỏi masterSet
        clients[index].active = false;
        clientCount--;
        return;
    }

    buff[bytesRead] = '\0';
    printf("Received from client %d: %s\n", index, buff);

    if (clients[index].loginStatus == 0) {
        // Nhập username
        int accountIndex = findAccount(accounts, *accountCount, buff);
        if (accountIndex == -1) {
            send(clients[index].fd, "Username not found", strlen("Username not found"), 0);
        } else if (accounts[accountIndex].status == 0) {
            send(clients[index].fd, "Account is blocked", strlen("Account is blocked"), 0);
        } else {
            strcpy(clients[index].username, buff);
            send(clients[index].fd, "Username accepted, please enter password", strlen("Username accepted, please enter password"), 0);
            clients[index].loginStatus = 1;
        }
    } else if (clients[index].loginStatus == 1) {
        // Nhập password
        int accountIndex = findAccount(accounts, *accountCount, clients[index].username);
        if (accountIndex != -1 && strcmp(accounts[accountIndex].password, buff) == 0) {
            send(clients[index].fd, "Login success", strlen("Login success"), 0);
            clients[index].loginStatus = 2;
        } else {
            send(clients[index].fd, "Wrong password", strlen("Wrong password"), 0);
        }
    } else if (clients[index].loginStatus == 2) {
        // Menu
        if (strcmp(buff, "1") == 0) {
            send(clients[index].fd, "Send new password", strlen("Send new password"), 0);
        } else if (strcmp(buff, "bye") == 0) {
            send(clients[index].fd, "Goodbye", strlen("Goodbye"), 0);
            close(clients[index].fd);
            FD_CLR(clients[index].fd, masterSet);  // Loại bỏ client khỏi masterSet
            clients[index].active = false;
            clientCount--;
        } else {
            // Cập nhật mật khẩu
            if (updatePassword(accounts, accountCount, clients[index].username, buff) == 0) {
                send(clients[index].fd, "Password updated", strlen("Password updated"), 0);
            } else {
                send(clients[index].fd, "Failed to update password", strlen("Failed to update password"), 0);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
        exit(1);
    }

    int listenfd, maxfd;
    struct sockaddr_in servaddr;
    fd_set masterSet, readySet;
    Account accounts[MAX_CLIENTS];
    int accountCount;

    if (loadAccounts(accounts, &accountCount) < 0) {
        exit(1);
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(listenfd, 10);

    FD_ZERO(&masterSet);
    FD_SET(listenfd, &masterSet);
    maxfd = listenfd;

    initClients();

    printf("Server is running on port %s\n", argv[1]);

    while (1) {
        readySet = masterSet;

        if (select(maxfd + 1, &readySet, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            break;
        }

        for (int i = 0; i <= maxfd; i++) {
            if (FD_ISSET(i, &readySet)) {
                if (i == listenfd) {
                    struct sockaddr_in cliaddr;
                    socklen_t cliaddr_len = sizeof(cliaddr);
                    int connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
                    if (connfd < 0) continue;

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (!clients[j].active) {
                            clients[j].fd = connfd;
                            clients[j].loginStatus = 0;
                            clients[j].active = true;
                            FD_SET(connfd, &masterSet);
                            if (connfd > maxfd) maxfd = connfd;
                            break;
                        }
                    }
                } else {
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd == i) {
                            handleClientInput(j, accounts, &accountCount, &masterSet);
                            break;
                        }
                    }
                }
            }
        }
    }

    close(listenfd);
    return 0;
}
