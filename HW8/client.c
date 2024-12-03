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

// Hàm nhập username
void inputUserName() {
    printf("Enter to exit\n");
    while (true) {
        printf("Insert username: ");
        fgets(buff, BUFF_SIZE, stdin);

        if (buff[0] == '\n') {
            printf("Exit programming\n");
            exit(1);
        }
        buff[strlen(buff) - 1] = '\0'; // Xóa ký tự newline '\n'

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

// Hàm nhập password
void inputPassword() {
    printf("Enter to exit\n");
    while (true) {
        printf("Insert password: ");
        fgets(buff, BUFF_SIZE, stdin);

        if (buff[0] == '\n') {
            printf("Exit programming\n");
            exit(1);
        }
        buff[strlen(buff) - 1] = '\0'; // Xóa ký tự newline '\n'

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

// Hàm hiển thị menu
void displayMenu() {
    printf("\n--- Menu ---\n");
    printf("1. Change password\n");
    printf("2. Bye\n");
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

    // Kiểm tra địa chỉ IP hợp lệ
    struct in_addr ipv4;
    if (inet_pton(AF_INET, serv_IP, &ipv4) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", serv_IP);
        exit(1);
    }

    serv_PORT = atoi(argv[2]);

    // Tạo socket TCP
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return 0;
    }

    // Định nghĩa địa chỉ server
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serv_IP);
    servaddr.sin_port = htons(serv_PORT);

    // Kết nối tới server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        return 0;
    }

    int isLogin = 0;
    while (1) {
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

            if (strcmp(buff, "Username accepted, please enter password") == 0) {
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

                if (strcmp(buff, "Login success") == 0) {
                    isLogin = 1; 
                }
            }
        }

        if (isLogin == 1) {
            displayMenu();
            fgets(buff, BUFF_SIZE, stdin);
            buff[strlen(buff) - 1] = '\0'; 

            int choice = atoi(buff);

            if (choice == 1) {
                printf("Enter new password: ");
                fgets(buff, BUFF_SIZE, stdin);
                buff[strlen(buff) - 1] = '\0';

                len = strlen(buff);
                sendBytes = send(sockfd, buff, len, 0);
                if (sendBytes < 0) {
                    perror("Error sending new password");
                    return 0;
                }

                rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
                if (rcvBytes < 0) {
                    perror("Error receiving response");
                    return 0;
                }

                buff[rcvBytes] = '\0';
                printf("Reply from server: %s\n", buff);

            } else if (choice == 2) {
                strcpy(buff, "bye");
                sendBytes = send(sockfd, buff, strlen(buff), 0);
                if (sendBytes < 0) {
                    perror("Error sending bye");
                    return 0;
                }

                printf("Goodbye!\n");
                close(sockfd);
                exit(0);
            } else {
                printf("Invalid choice, please try again.\n");
            }
        }
    }

    close(sockfd);
    return 0;
}
