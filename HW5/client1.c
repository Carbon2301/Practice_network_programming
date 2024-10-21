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
        buff[strlen(buff) - 1] = '\0'; // Xóa ký t? newline '\n'

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
            printf("Username is valid: %s\n", buff);
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
        buff[strlen(buff) - 1] = '\0'; // Xóa ký t? newline '\n'

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
            printf("Password is valid: %s\n", buff);
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

void displayMenu() {
    printf("\n--- Menu ---\n");
    printf("1. Change password\n");
    printf("2. Homepage\n");
    printf("3. Bye\n");
    printf("Enter your choice: ");
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

    // Ki?m tra d?a ch? IP h?p l?
    struct in_addr ipv4;
    if (inet_pton(AF_INET, serv_IP, &ipv4) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", serv_IP);
        exit(1);
    }

    serv_PORT = atoi(argv[2]);

    // Ki?m tra c?ng h?p l?
    if (!isValidPort(argv[2])) {
        fprintf(stderr, "Invalid port number: %s\n", argv[2]);
        exit(1);
    }

    // T?o socket TCP
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return 0;
    }

    // Ð?nh nghia d?a ch? server
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serv_IP);
    servaddr.sin_port = htons(serv_PORT);

    // K?t n?i t?i server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        return 0;
    }

    int isLogin = 0;
    do {
        if (isLogin == 0) {
            inputUserName();
            len = strlen(buff);
            sendBytes = send(sockfd, buff, len, 0);
            if (sendBytes < 0) {
                perror("Error sending username");
                return 0;
            }

            rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
            if (rcvBytes < 0) {
                perror("Error receiving response");
                return 0;
            }

            buff[rcvBytes] = '\0';
            printf("Reply from server: %s\n", buff);
            if (buff[0] == 'C') {
                continue;
            }
            if (buff[0] == 'A') {
                continue;
            }
        }

        while (isLogin == 0) {
            inputPassword();
            len = strlen(buff);
            sendBytes = send(sockfd, buff, len, 0);
            if (sendBytes < 0) {
                perror("Error sending password");
                return 0;
            }

            rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
            if (rcvBytes < 0) {
                perror("Error receiving response");
                return 0;
            }

            buff[rcvBytes] = '\0';
            printf("Reply from server: %s\n", buff);
            if (strcmp(buff, "OK") == 0) {
                isLogin = 1;
            }

            if (buff[0] == 'P' || buff[0] == 'C' || buff[0] == 'A') {
                break;
            }
        }

        while (isLogin == 1) {
            displayMenu();
            fgets(buff, BUFF_SIZE, stdin);
            buff[strlen(buff) - 1] = '\0';  // Xóa ký t? newline '\n'

            int choice = atoi(buff);

            if (choice == 1) {
                // Change password
                printf("Send new password to server: ");
                fgets(buff, BUFF_SIZE, stdin);
                buff[strlen(buff) - 1] = '\0';  // Xóa ký t? newline '\n'

                len = strlen(buff);
                sendBytes = send(sockfd, buff, len, 0);
                if (sendBytes < 0) {
                    perror("Error sending new password");
                    return 0;
                }

                // Nh?n ph?n h?i t? server sau khi thay d?i m?t kh?u
                rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
                if (rcvBytes < 0) {
                    perror("Error receiving response");
                    return 0;
                }

                buff[rcvBytes] = '\0';
                printf("Server says: %s\n", buff);

            } else if (choice == 2) {
                // Yêu c?u homepage t? server
                strcpy(buff, "homepage");
                sendBytes = send(sockfd, buff, strlen(buff), 0);
                if (sendBytes < 0) {
                    perror("Error requesting homepage");
                    return 0;
                }

                // Nh?n homepage t? server
                rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
                if (rcvBytes < 0) {
                    perror("Error receiving homepage");
                    return 0;
                }

                buff[rcvBytes] = '\0';
                printf("Homepage: %s\n", buff);

            } else if (choice == 3) {
                // G?i yêu c?u thoát
                strcpy(buff, "bye");
                sendBytes = send(sockfd, buff, strlen(buff), 0);
                if (sendBytes < 0) {
                    perror("Error sending bye");
                    return 0;
                }

                printf("Goodbye!\n");
                close(sockfd);  // Ðóng k?t n?i và thoát chuong trình
                exit(0);
            } else {
                // Hi?n th? l?i n?u ngu?i dùng nh?p không dúng l?a ch?n
                printf("Invalid choice, please try again.\n");
            }
        }
    } while (1);

    close(sockfd);
    return 0;
}

