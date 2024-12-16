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
    unsigned char *octets = (unsigned char *)&ipv4;
    for (int i = 0; i < 4; ++i) {
        if (octets[i] > 255) {
            fprintf(stderr, "Invalid value in octet %d: %d\n", i + 1, octets[i]);
            exit(1);
        }
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
      exit(1);
    }
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    
    if (getpeername(sockfd, (struct sockaddr*)&peer_addr, &peer_addr_len) == 0) {
        printf("Connected to server on port %d\n", ntohs(peer_addr.sin_port));
        if (ntohs(peer_addr.sin_port) != serv_PORT) {
            fprintf(stderr, "Error: Connected to the wrong server port. Expected %d, actual %d\n", serv_PORT, ntohs(peer_addr.sin_port));
             close(sockfd);
            exit(1);
        }
    } else {
        perror("getpeername error");
         close(sockfd);
        exit(1);
    }

    int isLogin = 0;
    do {
        if (isLogin == 0) {
            inputUserName();
            if (buff[0] == '\n'){
            	 exit(1);
			}
               
            len = strlen(buff);
            sendBytes = send(sockfd, buff, len, 0);
            rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
            printf("Reply from server: %s\n", buff);
            if (buff[0] == 'C')
                continue;
            if (buff[0] == 'A')
                continue;
            if (sendBytes < 0) {
                perror("Error 1");
                return 0;
            }
        }

        while (isLogin == 0) {
            inputPassword();
            len = strlen(buff);
            sendBytes = send(sockfd, buff, len, 0);
            if (sendBytes < 0) {
                perror("Error 1");
                return 0;
            }
            rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
            printf("Reply from server: %s\n", buff);
            if (strcmp(buff, "OK") == 0) {
                isLogin = 1;
            }
            if (buff[0] == 'P')
                break;
            if (buff[0] == 'C')
                break;
            if (buff[0] == 'A')
                break;
        }

        if (isLogin == 1) {
            printf("Send new password to server: ");
            fgets(buff, BUFF_SIZE, stdin);
            if (buff[0] == '\n'){
            	printf("Exit programing\n");
            	 exit(1);
			}
               	buff[strlen(buff) - 1] = '\0';
            len = 255; 
            sendBytes = send(sockfd, buff, len, 0);
            if (sendBytes < 0) {
                perror("Error 1");
                return 0;
            }
            for (;;) {
                rcvBytes = recv(sockfd, buff, BUFF_SIZE, 0);
                if (rcvBytes < 0) {
                    perror("Error 2");
                    return 0;
                }
                buff[rcvBytes] = '\0';
                if (buff[0] >= '0' && buff[0] <= '9')
				{
					printf("Reply from server(only number): %s\n", buff);
				}
				else if (strcmp(buff, "error") == 0)
				{
					printf("Reply from server: %s\n", buff);
				}
				else if (buff[0] != 'c' && buff[0] != 'G')
				{
					printf("Reply from server(only character): %s\n", buff);
				}

				if (strcmp(buff, "continue send message to server") == 0)
				{
					printf("%s\n", buff);
					printf("--------------------\n");
					break;
				}
				if (buff[0] == 'G')
				{
					printf("Reply from server %s\n", buff);
					printf("Signout\n");
					printf("--------------------\n");
					isLogin = 0;
					break;
				}
            }
        }
    } while (1);

    close(sockfd);
    return 0;
}
