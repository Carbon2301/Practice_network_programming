#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <ctype.h> 
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

// Hàm ki?m tra tr?ng thái tài kho?n c?a ngu?i dùng trong file
int checkUserStatus(char *username) {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Could not open file");
        exit(1);
    }

    char line[BUFF_SIZE];
    char fileUsername[50];
    char password[50];
    int status;
    char homepage[100];

    // Ð?c t?ng dòng trong file
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%s %s %d %s", fileUsername, password, &status, homepage);
        if (strcmp(fileUsername, username) == 0) {
            fclose(file);
            return status;  // Tr? v? status c?a tài kho?n
        }
    }

    fclose(file);
    return -1;  // Tr? v? -1 n?u không tìm th?y username
}


// Hàm ki?m tra m?t kh?u
bool checkPassword(User *user, char *password) {
    if (strcmp(user->password, password) == 0) {
        return true;
    }
    return false;
}

void handleClient(int connfd) {
    char buff[BUFF_SIZE];
    int rcvBytes, sendBytes;
    User *currentUser = NULL;
    int failedAttempts = 0;  // Ð?m s? l?n nh?p sai m?t kh?u

    while (1) {
        // Nh?n username t? client
        rcvBytes = recv(connfd, buff, BUFF_SIZE, 0);
        if (rcvBytes <= 0) {
            perror("Error receiving username");
            close(connfd);
            return;
        }
        buff[rcvBytes] = '\0'; // K?t thúc chu?i

        // Ki?m tra tr?ng thái tài kho?n c?a ngu?i dùng
        int status = checkUserStatus(buff);
        if (status == -1) {
            // Không tìm th?y user
            sendBytes = send(connfd, "Cannot find account", strlen("Cannot find account"), 0);
        } else if (status == 0) {
            // Tài kho?n b? khóa
            sendBytes = send(connfd, "Account is blocked", strlen("Account is blocked"), 0);
            continue;  // Yêu c?u nh?p l?i username
        } else {
            // Tài kho?n h?p l?
            sendBytes = send(connfd, "USER FOUND", strlen("USER FOUND"), 0);
            break;  // Thoát vòng l?p, ti?n hành ki?m tra m?t kh?u
        }
    }

    currentUser = searchUser(buff);  // Tìm ki?m ngu?i dùng trong danh sách
    if (currentUser == NULL) {
        close(connfd);
        return;
    }

    // Nh?n m?t kh?u t? client và ki?m tra
    while (failedAttempts < 3) {
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
            break;  // Ðang nh?p thành công, thoát vòng l?p
        } else {
            failedAttempts++;
            if (failedAttempts < 3) {
                sendBytes = send(connfd, "Password is incorrect. Try again", strlen("Password is incorrect. Try again"), 0);
            } else {
                // Khóa tài kho?n sau 3 l?n sai m?t kh?u
                currentUser->status = 0;
                saveUsersToFile();  // C?p nh?t thông tin tài kho?n vào file
                sendBytes = send(connfd, "Account is blocked due to 3 failed attempts", strlen("Account is blocked due to 3 failed attempts"), 0);
                close(connfd);
                return;  // Thoát chuong trình
            }
        }
    }

    if (failedAttempts < 3) {
        // X? lý ti?p t?c sau khi dang nh?p thành công
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
    }

    close(connfd);
}

// Hàm tách chu?i thành hai chu?i: m?t chu?i ch?a ch? cái và m?t chu?i ch?a ch? s?
void splitPassword(const char* password, char* letters, char* digits) {
    int letterIdx = 0, digitIdx = 0;
    for (int i = 0; i < strlen(password); i++) {
        if (isalpha(password[i])) {
            letters[letterIdx++] = password[i];
        } else if (isdigit(password[i])) {
            digits[digitIdx++] = password[i];
        }
    }
    letters[letterIdx] = '\0';  // K?t thúc chu?i ch? cái
    digits[digitIdx] = '\0';    // K?t thúc chu?i ch? s?
}

// Hàm c?p nh?t m?t kh?u m?i và tr? v? hai chu?i ký t? và ch? s?
void updatePassword(User *user, char *newPassword, int connfd) {
    // C?p nh?t m?t kh?u m?i cho user
    strcpy(user->password, newPassword);
    saveUsersToFile();  // Luu thông tin user vào file

    char letters[BUFF_SIZE] = {0};
    char digits[BUFF_SIZE] = {0};

    // Tách chu?i m?t kh?u thành chu?i ch? cái và chu?i ch? s?
    splitPassword(newPassword, letters, digits);

    // G?i ph?n h?i v? client: hai chu?i letters và digits
    char response[BUFF_SIZE];
    snprintf(response, sizeof(response), "Letters: %s, Digits: %s", letters, digits);
    send(connfd, response, strlen(response), 0);
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

