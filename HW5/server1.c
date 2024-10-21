#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>

#define BUFF_SIZE 255
#define FILENAME "nguoidung.txt"

// C?u trúc luu thông tin ngu?i dùng
typedef struct User {
    char username[50];
    char password[50];
    int status;    // 1: active, 0: blocked
    char homepage[100];
    struct User *next;
} User;

User *head = NULL;

// Hàm d?c thông tin ngu?i dùng t? file
void loadUsersFromFile() {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Could not open file");
        exit(1);
    }

    char line[BUFF_SIZE];
    while (fgets(line, sizeof(line), file)) {
        User *newUser = (User *)malloc(sizeof(User));
        sscanf(line, "%s %s %d %s", newUser->username, newUser->password, &newUser->status, newUser->homepage);
        newUser->next = head;
        head = newUser;
    }

    fclose(file);
}

// Hàm ghi l?i thông tin ngu?i dùng vào file
void saveUsersToFile() {
    FILE *file = fopen(FILENAME, "w");
    if (file == NULL) {
        perror("Could not open file");
        exit(1);
    }

    User *current = head;
    while (current != NULL) {
        fprintf(file, "%s %s %d %s\n", current->username, current->password, current->status, current->homepage);
        current = current->next;
    }

    fclose(file);
}

// Hàm tìm ki?m ngu?i dùng theo username
User* searchUser(char *username) {
    User *current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Hàm ki?m tra m?t kh?u
bool checkPassword(User *user, char *password) {
    if (strcmp(user->password, password) == 0) {
        return true;
    }
    return false;
}

// Hàm tách chu?i thành hai chu?i: m?t ch? ch?a ch? cái và m?t ch? ch?a ch? s?
void splitPassword(const char* password, char* letters, char* digits) {
    int letterIdx = 0, digitIdx = 0;
    for (int i = 0; i < strlen(password); i++) {
        if (isalpha(password[i])) {
            letters[letterIdx++] = password[i];
        } else if (isdigit(password[i])) {
            digits[digitIdx++] = password[i];
        }
    }
    letters[letterIdx] = '\0';  // K?t thúc chu?i ch?
    digits[digitIdx] = '\0';    // K?t thúc chu?i s?
}

// Hàm c?p nh?t m?t kh?u m?i và tr? v? các chu?i ký t? và ch? s?
void updatePassword(User *user, char *newPassword, int connfd) {
    strcpy(user->password, newPassword);
    saveUsersToFile();

    char letters[BUFF_SIZE] = {0};
    char digits[BUFF_SIZE] = {0};

    // Tách chu?i thành ký t? và ch? s?
    splitPassword(newPassword, letters, digits);

    // G?i ph?n h?i v? client: hai chu?i letters và digits
    char response[BUFF_SIZE];
    snprintf(response, sizeof(response), "Letters: %s, Digits: %s", letters, digits);
    send(connfd, response, strlen(response), 0);
}

// Hàm x? lý client
void handleClient(int connfd) {
    char buff[BUFF_SIZE];
    int rcvBytes, sendBytes;
    User *currentUser = NULL;

    // Nh?n username t? client
    rcvBytes = recv(connfd, buff, BUFF_SIZE, 0);
    if (rcvBytes <= 0) {
        perror("Error receiving username");
        close(connfd);
        return;
    }
    buff[rcvBytes] = '\0'; // K?t thúc chu?i

    currentUser = searchUser(buff);
    if (currentUser == NULL) {
        // Không tìm th?y user
        sendBytes = send(connfd, "Cannot find account", strlen("Cannot find account"), 0);
        close(connfd);
        return;
    } else if (currentUser->status == 0) {
        // Tài kho?n b? khóa
        sendBytes = send(connfd, "Account is blocked", strlen("Account is blocked"), 0);
        close(connfd);
        return;
    } else {
        // Tài kho?n h?p l?
        sendBytes = send(connfd, "USER FOUND", strlen("USER FOUND"), 0);
    }

    // Nh?n m?t kh?u t? client
    rcvBytes = recv(connfd, buff, BUFF_SIZE, 0);
    if (rcvBytes <= 0) {
        perror("Error receiving password");
        close(connfd);
        return;
    }
    buff[rcvBytes] = '\0'; // K?t thúc chu?i

    if (checkPassword(currentUser, buff)) {
        // M?t kh?u dúng, cho phép x? lý các tùy ch?n trong menu
        sendBytes = send(connfd, "OK", strlen("OK"), 0);

        while (1) {
            // Nh?n yêu c?u t? client
            rcvBytes = recv(connfd, buff, BUFF_SIZE, 0);
            if (rcvBytes <= 0) {
                perror("Error receiving option");
                close(connfd);
                return;
            }
            buff[rcvBytes] = '\0';

            if (strcmp(buff, "homepage") == 0) {
                // G?i homepage c?a ngu?i dùng
                send(connfd, currentUser->homepage, strlen(currentUser->homepage), 0);
            } else if (strcmp(buff, "bye") == 0) {
                // Thoát kh?i chuong trình
                printf("User %s disconnected.\n", currentUser->username);
                break;
            } else {
                // X? lý d?i m?t kh?u
                updatePassword(currentUser, buff, connfd);
            }
        }

    } else {
        // M?t kh?u sai
        sendBytes = send(connfd, "Password is incorrect", strlen("Password is incorrect"), 0);
        close(connfd);
    }

    close(connfd);
}

int main(int argc, char *argv[]) {
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len = sizeof(cliaddr);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Port>\n", argv[0]);
        exit(1);
    }

    short serv_PORT = atoi(argv[1]);

    // T?i danh sách ngu?i dùng t? file
    loadUsersFromFile();

    // T?o socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // C?u hình d?a ch? server
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(serv_PORT);

    // Bind socket
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(listenfd);
        exit(1);
    }

    // L?ng nghe k?t n?i
    if (listen(listenfd, 5) < 0) {
        perror("Listen failed");
        close(listenfd);
        exit(1);
    }

    printf("Server is running on port %d\n", serv_PORT);

    // Ch?p nh?n và x? lý k?t n?i
    while (1) {
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        if (connfd < 0) {
            perror("Accept failed");
            continue;
        }

        // X? lý client
        handleClient(connfd);
    }

    // Gi?i phóng tài nguyên
    close(listenfd);
    return 0;
}

