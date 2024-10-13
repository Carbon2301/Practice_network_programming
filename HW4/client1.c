#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IPAddress> <PortNumber>\n", argv[0]);
        return 1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[MAX_BUFFER];
    char username[50], password[50];

    // Tao socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Thiet lap dia chi cho server
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    int n;
    socklen_t len = sizeof(servaddr);

    // Giai doan 1: dang nhap
    while (1) {
        printf("Enter username: ");
        scanf("%s", username);
        printf("Enter password: ");
        scanf("%s", password);

        snprintf(buffer, sizeof(buffer), "%s %s", username, password);
        sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&servaddr, len);

        // Nhan phan hoi tu server
        n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, NULL, NULL);
        buffer[n] = '\0';  // Ð?t ký t? k?t thúc chu?i
        printf("Server: %s\n", buffer);

        if (strcmp(buffer, "OK") == 0) {
            printf("Login successful!\n");
            break;  
        } else if (strcmp(buffer, "Account is blocked") == 0) {
            printf("Account has been blocked after multiple failed attempts.\n");
            close(sockfd);
            return 1;
        } else if (strcmp(buffer, "Not OK") == 0) {
            printf("Incorrect password. Try again.\n");
        } else if (strcmp(buffer, "User not found") == 0) {
            printf("Username does not exist.\n");
        } else if (strcmp(buffer, "Account not ready") == 0) {
            printf("Account is blocked. Contact admin.\n");
            close(sockfd);
            return 1;
        }
    }

    
    getchar();

    // Giai doan 2: xu li lenh sau dang nhap
    while (1) {
        printf("Enter command (change_password / homepage / bye): ");
        fgets(buffer, sizeof(buffer), stdin); 

        buffer[strcspn(buffer, "\n")] = 0;

        sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&servaddr, len);

        if (strcmp(buffer, "bye") == 0) {
            n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, NULL, NULL);
            buffer[n] = '\0';
            printf("Server: %s\n", buffer);
            if (strcmp(buffer, "Goodbye") == 0) {
                printf("Logged out successfully.\n");
                break;
            }
        }
        
        else if (strcmp(buffer, "homepage") == 0) {
            n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, NULL, NULL);
            buffer[n] = '\0';
            printf("Server: Your homepage is %s\n", buffer);
        }

        else if (strcmp(buffer, "change_password") == 0) {
            char new_password[50];
            printf("Enter new password: ");
            scanf("%s", new_password);
            getchar();  

            sendto(sockfd, (const char *)new_password, strlen(new_password), 0, (const struct sockaddr *)&servaddr, len);

            // Nhan lai 2 chuoi ki tu va so
            char letters[MAX_BUFFER], digits[MAX_BUFFER];
            n = recvfrom(sockfd, (char *)letters, MAX_BUFFER, 0, NULL, NULL);
            letters[n] = '\0';
            n = recvfrom(sockfd, (char *)digits, MAX_BUFFER, 0, NULL, NULL);
            digits[n] = '\0';

            if (strcmp(letters, "Error: Invalid password") == 0) {
                printf("Server: %s\n", letters);
            } else {
                printf("Server: Password changed successfully!\n");
                printf("Letters: %s\n", letters);
                printf("Digits: %s\n", digits);
            }
        } else {
            printf("Invalid command. Please try again.\n");
        }
    }

    close(sockfd);
    return 0;
}

