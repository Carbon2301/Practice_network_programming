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

// C?u tr�c luu th�ng tin ngu?i d�ng
typedef struct User {
    char username[50];
    char password[50];
    int status;    // 1: active, 0: blocked
    char homepage[100];
    struct User *next;
} User;

User *head = NULL;

// H�m d?c th�ng tin ngu?i d�ng t? file
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

// H�m ghi l?i th�ng tin ngu?i d�ng v�o file
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

// H�m t�m ki?m ngu?i d�ng theo username
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

// H�m ki?m tra tr?ng th�i t�i kho?n c?a ngu?i d�ng trong file
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

    // �?c t?ng d�ng trong file
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%s %s %d %s", fileUsername, password, &status, homepage);
        if (strcmp(fileUsername, username) == 0) {
            fclose(file);
            return status;  // Tr? v? status c?a t�i kho?n
        }
    }

    fclose(file);
    return -1;  // Tr? v? -1 n?u kh�ng t�m th?y username
}


// H�m ki?m tra m?t kh?u
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
    int failedAttempts = 0;  // �?m s? l?n nh?p sai m?t kh?u

    while (1) {
        // Nh?n username t? client
        rcvBytes = recv(connfd, buff, BUFF_SIZE, 0);
        if (rcvBytes <= 0) {
            perror("Error receiving username");
            close(connfd);
            return;
        }
        buff[rcvBytes] = '\0'; // K?t th�c chu?i

        // Ki?m tra tr?ng th�i t�i kho?n c?a ngu?i d�ng
        int status = checkUserStatus(buff);
        if (status == -1) {
            // Kh�ng t�m th?y user
            sendBytes = send(connfd, "Cannot find account", strlen("Cannot find account"), 0);
        } else if (status == 0) {
            // T�i kho?n b? kh�a
            sendBytes = send(connfd, "Account is blocked", strlen("Account is blocked"), 0);
            continue;  // Y�u c?u nh?p l?i username
        } else {
            // T�i kho?n h?p l?
            sendBytes = send(connfd, "USER FOUND", strlen("USER FOUND"), 0);
            break;  // Tho�t v�ng l?p, ti?n h�nh ki?m tra m?t kh?u
        }
    }

    currentUser = searchUser(buff);  // T�m ki?m ngu?i d�ng trong danh s�ch
    if (currentUser == NULL) {
        close(connfd);
        return;
    }

    // Nh?n m?t kh?u t? client v� ki?m tra
    while (failedAttempts < 3) {
        rcvBytes = recv(connfd, buff, BUFF_SIZE, 0);
        if (rcvBytes <= 0) {
            perror("Error receiving password");
            close(connfd);
            return;
        }
        buff[rcvBytes] = '\0'; // K?t th�c chu?i

        if (checkPassword(currentUser, buff)) {
            // M?t kh?u d�ng, cho ph�p x? l� c�c t�y ch?n trong menu
            sendBytes = send(connfd, "OK", strlen("OK"), 0);
            break;  // �ang nh?p th�nh c�ng, tho�t v�ng l?p
        } else {
            failedAttempts++;
            if (failedAttempts < 3) {
                sendBytes = send(connfd, "Password is incorrect. Try again", strlen("Password is incorrect. Try again"), 0);
            } else {
                // Kh�a t�i kho?n sau 3 l?n sai m?t kh?u
                currentUser->status = 0;
                saveUsersToFile();  // C?p nh?t th�ng tin t�i kho?n v�o file
                sendBytes = send(connfd, "Account is blocked due to 3 failed attempts", strlen("Account is blocked due to 3 failed attempts"), 0);
                close(connfd);
                return;  // Tho�t chuong tr�nh
            }
        }
    }

    if (failedAttempts < 3) {
        // X? l� ti?p t?c sau khi dang nh?p th�nh c�ng
        while (1) {
            // Nh?n y�u c?u t? client
            rcvBytes = recv(connfd, buff, BUFF_SIZE, 0);
            if (rcvBytes <= 0) {
                perror("Error receiving option");
                close(connfd);
                return;
            }
            buff[rcvBytes] = '\0';

            if (strcmp(buff, "homepage") == 0) {
                // G?i homepage c?a ngu?i d�ng
                send(connfd, currentUser->homepage, strlen(currentUser->homepage), 0);
            } else if (strcmp(buff, "bye") == 0) {
                // Tho�t kh?i chuong tr�nh
                printf("User %s disconnected.\n", currentUser->username);
                break;
            } else {
                // X? l� d?i m?t kh?u
                updatePassword(currentUser, buff, connfd);
            }
        }
    }

    close(connfd);
}

// H�m t�ch chu?i th�nh hai chu?i: m?t chu?i ch?a ch? c�i v� m?t chu?i ch?a ch? s?
void splitPassword(const char* password, char* letters, char* digits) {
    int letterIdx = 0, digitIdx = 0;
    for (int i = 0; i < strlen(password); i++) {
        if (isalpha(password[i])) {
            letters[letterIdx++] = password[i];
        } else if (isdigit(password[i])) {
            digits[digitIdx++] = password[i];
        }
    }
    letters[letterIdx] = '\0';  // K?t th�c chu?i ch? c�i
    digits[digitIdx] = '\0';    // K?t th�c chu?i ch? s?
}

// H�m c?p nh?t m?t kh?u m?i v� tr? v? hai chu?i k� t? v� ch? s?
void updatePassword(User *user, char *newPassword, int connfd) {
    // C?p nh?t m?t kh?u m?i cho user
    strcpy(user->password, newPassword);
    saveUsersToFile();  // Luu th�ng tin user v�o file

    char letters[BUFF_SIZE] = {0};
    char digits[BUFF_SIZE] = {0};

    // T�ch chu?i m?t kh?u th�nh chu?i ch? c�i v� chu?i ch? s?
    splitPassword(newPassword, letters, digits);

    // G?i ph?n h?i v? client: hai chu?i letters v� digits
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

    // T?i danh s�ch ngu?i d�ng t? file
    loadUsersFromFile();

    // T?o socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // C?u h�nh d?a ch? server
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

    // Ch?p nh?n v� x? l� k?t n?i
    while (1) {
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        if (connfd < 0) {
            perror("Accept failed");
            continue;
        }

        // X? l� client
        handleClient(connfd);
    }

    // Gi?i ph�ng t�i nguy�n
    close(listenfd);
    return 0;
}

