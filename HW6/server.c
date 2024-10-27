#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>

#define FILENAME "account.txt"
#define BUFF_SIZE 255

int listenfd, connfd;
int flag = 0;

char buff[BUFF_SIZE];
char recvBuff[BUFF_SIZE];
struct sockaddr_in servaddr, cliaddr;
unsigned int len = sizeof(struct sockaddr_in);
short serv_PORT;

typedef struct User {
    char username[50];
    char password[50];
    int status;
    int attempt;
    struct User* next;
} User;

User* head = NULL;
User* currentUser = NULL;

User* searchUser(char* username) {
    User* current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void loadUsersFromFile() {
    FILE* file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Khong the mo file");
        exit(1);
    }
    while (head != NULL) {
        User* temp = head;
        head = head->next;
        free(temp);
    }
    char line[150];
    while (fgets(line, sizeof(line), file)) {
        User* newUser = (User*)malloc(sizeof(User));
        sscanf(line, "%s %s %d", newUser->username, newUser->password, &newUser->status);
        newUser->attempt = 0;
        newUser->next = head;
        head = newUser;
    }
    fclose(file);
}

void saveUsersToFile() {
    FILE* file = fopen(FILENAME, "w");
    if (file == NULL) {
        perror("Cannot open file!");
        exit(1);
    }
    User* current = head;
    while (current != NULL) {
        fprintf(file, "%s %s %d\n", current->username, current->password, current->status);
        current = current->next;
    }
    fclose(file);
}

/*Chuc nang kiem tra mat khau*/
void checkWrongAttempts(User* user, char* enteredPassword) {
    if (strcmp(enteredPassword, user->password) == 0) {
        send(connfd, "OK", BUFF_SIZE, 0);
        currentUser = user;
        flag = 2;
        return;
    } else {
        user->attempt++;
        if (user->attempt >= 3) {
            user->status = 0;
            saveUsersToFile();
            send(connfd, "Password is incorrect. Account is blocked\n", BUFF_SIZE, 0);
        } else {
            send(connfd, "NOT OK", BUFF_SIZE, 0);
        }
    }
}

void sig_chld(int signo){
    pid_t pid;
    int stat;
    pid = waitpid(-1, &stat, WNOHANG);
    printf("Child %d terminated\n", pid);
}

int main(int argc, char* argv[]) {
    loadUsersFromFile();
    len = sizeof(cliaddr);
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <TCP SERVER PORT>\n", argv[0]);
        exit(1);
    }

    serv_PORT = atoi(argv[1]);
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: ");
        return 0;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(serv_PORT);

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Error: ");
        return 0;
    }

    if (listen(listenfd, 3) < 0) {
        perror("Error: ");
        return 0;
    }
    signal(SIGCHLD, sig_chld);

    printf("Server started!\n");

    while (1) {
        if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len)) < 0) {
            perror("Error: ");
            return 0;
        }

        pid_t pid = fork();
        if (pid == 0) { // Tien trinh con
            close(listenfd);
            char username[50], password[50];

            while (1) {
                memset(recvBuff, 0, sizeof(recvBuff));
                int rcvSize = recv(connfd, recvBuff, BUFF_SIZE, 0);

                if (rcvSize <= 0) {
                    break; // Client disconnected
                }

                recvBuff[rcvSize] = '\0';
                if (flag == 0) { // Kiem tra user name
                    strcpy(username, recvBuff);
                    currentUser = searchUser(username);
                    if (currentUser != NULL && currentUser->status == 1) {
                        send(connfd, "USER FOUND", BUFF_SIZE, 0);
                        flag = 1;
                    } else if (currentUser != NULL && currentUser->status == 0) {
                        send(connfd, "Account is blocked", BUFF_SIZE, 0);
                    } else {
                        send(connfd, "Cannot find account", BUFF_SIZE, 0);
                    }
                } else if (flag == 1) { // Kiem tra mat khau
                    strcpy(password, recvBuff);
                    checkWrongAttempts(currentUser, password);
                    if (flag == 2) {
                        send(connfd, "Enter Message", BUFF_SIZE, 0);
                    }
                } else { // Sau khi dang nhap thanh cong
                    printf("User %s sent: %s\n", currentUser->username, recvBuff);
                    send(connfd, "Message received", BUFF_SIZE, 0);
                }
            }
            close(connfd);
            exit(0);
        } else {
            close(connfd);
        }
    }
    close(listenfd);
    return 0;
}
