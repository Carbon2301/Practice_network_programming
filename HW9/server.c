#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>

#define FILENAME "account.txt"
#define BUFF_SIZE 255
#define BACKLOG 20
 struct pollfd fds[BACKLOG + 1];
fd_set readfds, allset;
socklen_t clilen;
int nready, client[BACKLOG];
char buff[BUFF_SIZE];
char recvBuff[BUFF_SIZE];
char done[BUFF_SIZE];
char username[50], password[50];
unsigned int len = sizeof(struct sockaddr_in);
short serv_PORT;
 int useClient = 0;

typedef struct User {
    char username[50];
    char password[50];
    int status;
    int attempt;
    struct User *next;
} User;

User *head = NULL;
int flag[BACKLOG];
User *currentUser[BACKLOG];
int isLoggedIn[BACKLOG];
char loggedInUsername[BACKLOG][50];
int error[BACKLOG];

char number[BUFF_SIZE];
char alphabet[BUFF_SIZE];


void loadUsersFromFile() {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Khong the mo file");
        exit(1);
    }
    char line[150];
    while (head != NULL) {
        User *temp = head;
        head = head->next;
        free(temp);
    }
    while (fgets(line, sizeof(line), file)) {
        User *newUser = (User *) malloc(sizeof(User));
        sscanf(line, "%s %s %d", newUser->username, newUser->password, &newUser->status);
        newUser->attempt = 0;
        newUser->next = head;
        head = newUser;
    }
    fclose(file);
}

/*ghi thong tin ra file*/
void saveUsersToFile() {
    FILE *file = fopen(FILENAME, "w");
    if (file == NULL) {
        perror("Khong the mo file");
        exit(1);
    }

    User *current = head;
    while (current != NULL) {
        fprintf(file, "%s %s %d  \n", current->username, current->password, current->status);
        current = current->next;
    }
    fclose(file);
}

void processRecvBuff(const char *receive, int i) {
    int countNumber = 0, countAlphabet = 0;
    for (size_t k = 0; k < strlen(receive); k++) {
        char currentChar = receive[k];

        if (isdigit(currentChar)) {
            if (countNumber < BUFF_SIZE - 1) {
                number[countNumber++] = currentChar;
            }
        } else if (isalpha(currentChar)) {
            if (countAlphabet < BUFF_SIZE - 1) {
                alphabet[countAlphabet++] = currentChar;
            }
        } else {
            error[i] = 1;
            break;
        }
    }
    if (error[i] == 0) {
        strcpy(currentUser[i]->password, receive);
        saveUsersToFile();
    }
    number[countNumber] = '\0';
    alphabet[countAlphabet] = '\0';
}

void checkWrongAttempts(User *user, char *enteredPassword, int connfd, int i) {
    int sendSize;
    if (strcmp(enteredPassword, user->password) == 0) {
        send(connfd, "OK", BUFF_SIZE, 0);
        isLoggedIn[i] = 1;
        flag[i] = 2;
        currentUser[i] = user;
        return;
    }
    user->attempt++;
    if (user->attempt >= 3) {
        user->status = 0;
        flag[i] = 0;
        saveUsersToFile();
        sendSize = send(connfd, "Password is incorrect. Account is blocked\n", BUFF_SIZE, 0);
        if (sendSize <= 0) {
             close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
        }
    } else {
        sendSize = send(connfd, "NOT OK", BUFF_SIZE, 0);
        if (sendSize <= 0) {
            close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
        }
    }
}

User *searchUser(char *username) {
    User *current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int isValidPort(const char *portStr) {
    for (int i = 0; portStr[i] != '\0'; ++i) {
        if (!isdigit(portStr[i])) {
            return 0;  
        }
    }

    int port = atoi(portStr);
    if (port < 0 || port > 65535) {
        return 0;  
    }

    if (portStr[0] == '0' && portStr[1] != '\0') {
        return 0;  
    }

    return 1;  
}

int maxfd, maxi;

void handleDataFromClient(int connfd, int i) {
    memset(buff, '\0', sizeof(buff));
    memset(done, '\0', sizeof(done));

    int rcvSize = recv(connfd, recvBuff, BUFF_SIZE, 0); 
    if (rcvSize <= 0) {
        printf("Client %d disconnected\n", connfd); 
       
        close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--;                                
        client[i] = -1;
        flag[i] = 0;
        error[i] = 0;
        isLoggedIn[i] = 0;
        currentUser[i] = NULL;
                        
        return;
    }

    int sendSize;
    recvBuff[rcvSize] = '\0';
    printf("%s\n", recvBuff);
    if (flag[i] == 0) {
        strcpy(username, recvBuff);
        currentUser[i] = searchUser(username);
        if (currentUser[i] != NULL) {
            if (currentUser[i]->status == 1) {
                sendSize = send(connfd, "USER FOUND", BUFF_SIZE, 0);
                if (sendSize <= 0) {
                     close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
                }
                flag[i] = 1;
            } else if (currentUser[i]->status == 2) {
                flag[i] = 0;
                sendSize = send(connfd, "Account is not activated\n", BUFF_SIZE, 0);
                if (sendSize <= 0) {
                    close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
                }
            } else if (currentUser[i]->status == 0) {
                flag[i] = 0;
                sendSize = send(connfd, "Account is blocked\n", BUFF_SIZE, 0);
                if (sendSize <= 0) {
                     close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
                }
            }
        } else {
            sendSize = send(connfd, "Cannot find account\n", BUFF_SIZE, 0);
            if (sendSize <= 0) {
               close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
            }
        }
        return;
    } else if (flag[i] == 1) {
        strcpy(password, recvBuff);
        checkWrongAttempts(currentUser[i], password, connfd, i);
    } else {
        if (rcvSize < 0) {
            perror("Error: ");

        }
        recvBuff[rcvSize] = '\0';
        if (strcmp(recvBuff, "bye") == 0) {
            char str1[100] = "Goodbye";
            char str2[100];
            strcpy(str2, currentUser[i]->username);
            currentUser[i]->attempt = 0;
            char str3[100];
            strcpy(str3, str1);
            strcat(str3, str2);
            sendSize = send(connfd, str3, BUFF_SIZE, 0);
            if (sendSize <= 0) {
               close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
            }
            flag[i] = 0;
            return;
        }
        printf("Receive from client: %s\n", recvBuff);
        processRecvBuff(recvBuff, i);
        if (error[i] == 1) {
            strcpy(buff, "error");
            sendSize = send(connfd, buff, BUFF_SIZE, 0);
            if (sendSize <= 0) {
               close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
            }
            strcpy(done, "continue send message to server");
            send(connfd, done, BUFF_SIZE, 0);
            error[i] = 0;
        } else {
            if (number[0] != '\0') {
                
                sendSize = send(connfd, number, BUFF_SIZE, 0);
                if (sendSize <= 0) {
                    close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
                }
            }
            if (alphabet[0] != '\0') {
                sendSize = send(connfd, alphabet, BUFF_SIZE, 0);
                if (sendSize <= 0) {
                    close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
                }
            }
            strcpy(done, "continue send message to server");
            sendSize = send(connfd, done, BUFF_SIZE, 0);
            if (sendSize <= 0) {
                  close(connfd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        useClient--; 
            }
        }
    }
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < BACKLOG; i++) {
        flag[i] = 0;
        currentUser[i] = NULL;
        isLoggedIn[i] = 0;
        error[i] = 0;
    }

    int listenfd, connfd, sockfd, i;

    struct sockaddr_in cliaddr, servaddr;

    loadUsersFromFile();

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <TCP SERVER PORT>\n", argv[0]);
        exit(1);
    }

    serv_PORT = atoi(argv[1]);
    if (!isValidPort(argv[1])) {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        exit(1);
    }

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: ");
        return 0;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(serv_PORT);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("Error: ");
        return 0;
    }

    if (listen(listenfd, BACKLOG) < 0) {
        perror("Error: ");
        return 0;
    }

    maxfd = listenfd;
    maxi = -1;
    for (i = 0; i < BACKLOG; i++)
        client[i] = -1;

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listenfd;
    fds[0].events = POLLIN;
    printf("Server started!\n");
       
    while (1) {
      
         int nready = poll(fds, useClient + 1, 5000);
        
            if ( fds[0].revents & POLLIN) {
                        clilen = sizeof(cliaddr);
                        int connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                        if (connfd < 0)
                            perror("\nError: ");
                        else {
                            printf("You got a connection from %s\n", inet_ntoa(cliaddr.sin_addr));
                            for (int i = 1; i <BACKLOG; i++) {
                                 if (fds[i].fd == 0)
                                    {

                                        fds[i].fd = connfd;
                                        fds[i].events = POLLIN ;
                                        useClient++;
                                        break;
                                    }
                            }
                            
                        }
                       
                    }

                for (int i = 1; i < BACKLOG; i++) {
                    if (fds[i].fd > 0 && fds[i].revents & POLLIN) {
                        int sockfd = fds[i].fd;
                        handleDataFromClient(sockfd, i);
                    }
                }
            }

    close(listenfd);
    return 0;
}
