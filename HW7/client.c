#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

#define BUFF_SIZE 255

char buff[BUFF_SIZE];
int len;

void inputUserName() {
    printf("Enter to exit\n");
    while (true) {
        printf("Insert username: ");
        fgets(buff, BUFF_SIZE, stdin);

        if (buff[0] == '\n') {
            printf("Exit programming\n");
            exit(1);
        }
        buff[strlen(buff) - 1] = '\0';

        bool containsSpace = false;
        for (int i = 0; i < strlen(buff); i++) {
            if (buff[i] == ' ') {
                containsSpace = true;
                break;
            }
        }

        if (containsSpace) {
            printf("Username cannot contain spaces. Please try again.\n");
        } else {
            break;
        }
    }
}

void inputPassword() {
    printf("Enter to exit\n");
    while (true) {
        printf("Insert password: ");
        fgets(buff, BUFF_SIZE, stdin);

        if (buff[0] == '\n') {
            printf("Exit programming\n");
            exit(1);
        }
        buff[strlen(buff) - 1] = '\0';

        bool containsSpace = false;
        for (int i = 0; i < strlen(buff); i++) {
            if (buff[i] == ' ') {
                containsSpace = true;
                break;
            }
        }

        if (containsSpace) {
            printf("Password cannot contain spaces. Please try again.\n");
        } else {
            break;
        }
    }
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

int main(int argc, char *argv[]) {
    int sockfd, rcvBytes, sendBytes;
    struct sockaddr_in servaddr;
    char *serv_IP;
    short serv_PORT;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort>\n", argv[0]);
        exit(1);
    }

    serv_IP = argv[1];
    struct in_addr ipv4;
    if (inet_pton(AF_INET, serv_IP, &ipv4) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", serv_IP);
        exit(1);
    }

    serv_PORT = atoi(argv[2]);
    if (!isValidPort(argv[2])) {
        fprintf(stderr, "Invalid port number: %s\n", argv[2]);
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: ");
        return 0;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serv_IP);
    servaddr.sin_port = htons(serv_PORT);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Error: ");
        return 0;
    }

    int isLogin = 0;
    do {
        if (isLogin == 0) {
            inputUserName();
            len = strlen(buff);
            sendBytes = send(sockfd, buff, len, 0);
            rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
            printf("Reply from server: %s\n", buff);

            if (strcmp(buff, "USER FOUND") == 0) {
                isLogin = 1; 
            } else if (strcmp(buff, "Cannot find account") == 0 || strcmp(buff, "Account is blocked") == 0) {
                continue;
            }
        }

        while (isLogin == 1) {
            inputPassword();
            len = strlen(buff);
            sendBytes = send(sockfd, buff, len, 0);
            rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
            printf("Reply from server: %s\n", buff);

            if (strcmp(buff, "OK") == 0) {
                isLogin = 2; // Login successful
                printf("Enter Message\n"); 
                break;
            } else if (strcmp(buff, "Password is incorrect. Account is blocked\n") == 0) {
                isLogin = 0; // Reset to enter username again
                break;
            }
        }

        while (isLogin == 2) { // Da dang nhap
            printf("Enter message (type 'bye' to exit): ");
            fgets(buff, BUFF_SIZE, stdin);
            buff[strlen(buff) - 1] = '\0';

            if (strcmp(buff, "bye") == 0) {
                send(sockfd, buff, strlen(buff), 0);
                printf("Goodbye\n");
                isLogin = 0; // Reset to enter username if needed
                break;
            }

            sendBytes = send(sockfd, buff, strlen(buff), 0);
            if (sendBytes < 0) {
                perror("Error sending message");
                return 0;
            }

            rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
            if (rcvBytes < 0) {
                perror("Error receiving message");
                return 0;
            }

            buff[rcvBytes] = '\0';
            printf("Reply from server: %s\n", buff);
        }

    } while (1);

    close(sockfd);
    return 0;
}
