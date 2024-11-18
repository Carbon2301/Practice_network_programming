#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

#define FILENAME "account.txt"
#define BUFF_SIZE 255
#define MAX_CLIENTS 100

typedef struct User {
    char username[50];
    char password[50];
    int status;
    int attempt;
    struct User* next;
} User;

typedef struct ThreadArgs {
    int connfd;
    struct sockaddr_in cliaddr;
} ThreadArgs;

pthread_mutex_t userLock = PTHREAD_MUTEX_INITIALIZER;
User* head = NULL;

// Load user accounts from file
void loadUsersFromFile() {
    FILE* file = fopen(FILENAME, "r");
    if (!file) {
        perror("Cannot open file");
        exit(1);
    }
    while (head) {
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

// Save user accounts to file
void saveUsersToFile() {
    FILE* file = fopen(FILENAME, "w");
    if (!file) {
        perror("Cannot open file");
        exit(1);
    }

    pthread_mutex_lock(&userLock);
    User* current = head;
    while (current) {
        fprintf(file, "%s %s %d\n", current->username, current->password, current->status);
        current = current->next;
    }
    pthread_mutex_unlock(&userLock);

    fclose(file);
}

// Search for a user by username
User* searchUser(char* username) {
    pthread_mutex_lock(&userLock);
    User* current = head;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            pthread_mutex_unlock(&userLock);
            return current;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&userLock);
    return NULL;
}

// Thread handler for each client
void* clientHandler(void* args) {
    ThreadArgs* tArgs = (ThreadArgs*)args;
    int connfd = tArgs->connfd;
    free(tArgs);

    char buff[BUFF_SIZE], recvBuff[BUFF_SIZE];
    int isLogin = 0;
    User* currentUser = NULL;

    while (1) {
        memset(recvBuff, 0, BUFF_SIZE);
        int rcvSize = recv(connfd, recvBuff, BUFF_SIZE, 0);

        if (rcvSize <= 0) break;  // Client disconnected

        recvBuff[rcvSize] = '\0';

        if (isLogin == 0) {  // Check username
            currentUser = searchUser(recvBuff);
            if (currentUser && currentUser->status == 1) {
                send(connfd, "USER FOUND", BUFF_SIZE, 0);
                isLogin = 1;
            } else if (currentUser && currentUser->status == 0) {
                send(connfd, "Account is blocked", BUFF_SIZE, 0);
            } else {
                send(connfd, "Cannot find account", BUFF_SIZE, 0);
            }
        } else if (isLogin == 1) {  // Check password
            if (strcmp(recvBuff, currentUser->password) == 0) {
                send(connfd, "OK", BUFF_SIZE, 0);
                isLogin = 2;
            } else {
                currentUser->attempt++;
                if (currentUser->attempt >= 3) {
                    currentUser->status = 0;
                    saveUsersToFile();
                    send(connfd, "Password is incorrect. Account is blocked\n", BUFF_SIZE, 0);
                    isLogin = 0;
                } else {
                    send(connfd, "NOT OK", BUFF_SIZE, 0);
                }
            }
        } else {  // After successful login
            if (strcmp(recvBuff, "bye") == 0) {
                break;
            }
            printf("User %s sent: %s\n", currentUser->username, recvBuff);
            send(connfd, "Message received", BUFF_SIZE, 0);
        }
    }
    close(connfd);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
        exit(1);
    }

    loadUsersFromFile();

    int listenfd;
    struct sockaddr_in servaddr;
    short serv_PORT = atoi(argv[1]);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(serv_PORT);

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(listenfd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("Server is running on port %d\n", serv_PORT);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t cliLen = sizeof(cliaddr);
        int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &cliLen);

        if (connfd < 0) {
            perror("Accept failed");
            continue;
        }

        ThreadArgs* args = (ThreadArgs*)malloc(sizeof(ThreadArgs));
        args->connfd = connfd;
        args->cliaddr = cliaddr;

        pthread_t tid;
        pthread_create(&tid, NULL, clientHandler, (void*)args);
        pthread_detach(tid);
    }

    close(listenfd);
    return 0;
}

