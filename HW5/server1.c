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


typedef struct User {
    char username[50];
    char password[50];
    int status;    // 1: active, 0: blocked
    char homepage[100];
    struct User *next;
} User;

User *head = NULL;


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

// Hàm tìm kiếm người dùng theo username
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


    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%s %s %d %s", fileUsername, password, &status, homepage);
        if (strcmp(fileUsername, username) == 0) {
            fclose(file);
            return status; 
        }
    }

    fclose(file);
    return -1;  
}


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
    int failedAttempts = 0;  

    while (1) {

        rcvBytes = recv(connfd, buff, BUFF_SIZE, 0);
        if (rcvBytes <= 0) {
            perror("Error receiving username");
            close(connfd);
            return;
        }
        buff[rcvBytes] = '\0'; 


        int status = checkUserStatus(buff);
        if (status == -1) {
            sendBytes = send(connfd, "Cannot find account", strlen("Cannot find account"), 0);
        } else if (status == 0) {
            sendBytes = send(connfd, "Account is blocked", strlen("Account is blocked"), 0);
            continue;  
        } else {
            sendBytes = send(connfd, "USER FOUND", strlen("USER FOUND"), 0);
            break;  
        }
    }

    currentUser = searchUser(buff); 
    if (currentUser == NULL) {
        close(connfd);
        return;
    }


    while (failedAttempts < 3) {
        rcvBytes = recv(connfd, buff, BUFF_SIZE, 0);
        if (rcvBytes <= 0) {
            perror("Error receiving password");
            close(connfd);
            return;
        }
        buff[rcvBytes] = '\0';
        if (checkPassword(currentUser, buff)) {
            sendBytes = send(connfd, "OK", strlen("OK"), 0);
            break;  
        } else {
            failedAttempts++;
            if (failedAttempts < 3) {
                sendBytes = send(connfd, "Password is incorrect. Try again", strlen("Password is incorrect. Try again"), 0);
            } else {
                currentUser->status = 0;
                saveUsersToFile();  
                sendBytes = send(connfd, "Account is blocked due to 3 failed attempts", strlen("Account is blocked due to 3 failed attempts"), 0);
                close(connfd);
                return;  
            }
        }
    }

    if (failedAttempts < 3) {
        while (1) {
            rcvBytes = recv(connfd, buff, BUFF_SIZE, 0);
            if (rcvBytes <= 0) {
                perror("Error receiving option");
                close(connfd);
                return;
            }
            buff[rcvBytes] = '\0';

            if (strcmp(buff, "homepage") == 0) {
                send(connfd, currentUser->homepage, strlen(currentUser->homepage), 0);
            } else if (strcmp(buff, "bye") == 0) {
                printf("User %s disconnected.\n", currentUser->username);
                break;
            } else {

                updatePassword(currentUser, buff, connfd);
            }
        }
    }

    close(connfd);
}

// Hàm tách 2 chuỗi chữ cái và chữ số
void splitPassword(const char* password, char* letters, char* digits) {
    int letterIdx = 0, digitIdx = 0;
    for (int i = 0; i < strlen(password); i++) {
        if (isalpha(password[i])) {
            letters[letterIdx++] = password[i];
        } else if (isdigit(password[i])) {
            digits[digitIdx++] = password[i];
        }
    }
    letters[letterIdx] = '\0';  
    digits[digitIdx] = '\0';    
}


void updatePassword(User *user, char *newPassword, int connfd) {

    strcpy(user->password, newPassword);
    saveUsersToFile();  

    char letters[BUFF_SIZE] = {0};
    char digits[BUFF_SIZE] = {0};

    splitPassword(newPassword, letters, digits);

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

    loadUsersFromFile();

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(serv_PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(listenfd);
        exit(1);
    }

    if (listen(listenfd, 5) < 0) {
        perror("Listen failed");
        close(listenfd);
        exit(1);
    }

    printf("Server is running on port %d\n", serv_PORT);

    while (1) {
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        if (connfd < 0) {
            perror("Accept failed");
            continue;
        }

        handleClient(connfd);
    }

    close(listenfd);
    return 0;
}

