#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Su dung: %s <IPAddress> <PortNumber>\n", argv[0]);
        return 1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[MAX_BUFFER];
    char username[50], password[50];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Tao socket that bai");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    int n;
    socklen_t len = sizeof(servaddr);

    // Giai doan 1: Dang nhap
    while (1) {
        printf("Nhap username: ");
        scanf("%s", username);
        printf("Nhap password: ");
        scanf("%s", password);

        snprintf(buffer, sizeof(buffer), "%s %s", username, password);
        sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&servaddr, len);

        n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, NULL, NULL);
        buffer[n] = '\0';
        printf("Server: %s\n", buffer);

        if (strcmp(buffer, "OK") == 0) {
            printf("Dang nhap thanh cong!\n");
            break;
        } else if (strcmp(buffer, "Account is blocked") == 0) {
            printf("Tai khoan da bi khoa sau nhieu lan sai.\n");
            close(sockfd);
            return 1;
        } else if (strcmp(buffer, "Not OK") == 0) {
            printf("Mat khau sai. Thu lai.\n");
        } else if (strcmp(buffer, "User not found") == 0) {
            printf("Khong ton tai username.\n");
        } else if (strcmp(buffer, "Account not ready") == 0) {
            printf("Tai khoan bi khoa. Lien he quan tri vien.\n");
            close(sockfd);
            return 1;
        }
    }

    // Giai doan 2: Thuc hien cac hanh dong sau khi dang nhap
    while (1) {
        printf("Nhap lenh (change_password / homepage / bye): ");
        scanf("%s", buffer);

        // Gui lenh toi server
        sendto(sockfd, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&servaddr, len);

        // Xu ly lenh "bye"
        if (strcmp(buffer, "bye") == 0) {
            n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, NULL, NULL);
            buffer[n] = '\0';
            printf("Server: %s\n", buffer);
            if (strcmp(buffer, "Goodbye") == 0) {
                printf("Dang xuat thanh cong.\n");
                break;
            }
        }
        // Xu ly lenh "homepage"
        else if (strcmp(buffer, "homepage") == 0) {
            n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER, 0, NULL, NULL);
            buffer[n] = '\0';
            printf("Server: Trang chu cua ban la %s\n", buffer);
        }
        // Xu ly lenh "change_password"
        else if (strcmp(buffer, "change_password") == 0) {
            char new_password[50];
            printf("Nhap mat khau moi: ");
            scanf("%s", new_password);

            sendto(sockfd, (const char *)new_password, strlen(new_password), 0, (const struct sockaddr *)&servaddr, len);

            // Nhan lai hai chuoi: chu cai va so
            char letters[MAX_BUFFER], digits[MAX_BUFFER];
            n = recvfrom(sockfd, (char *)letters, MAX_BUFFER, 0, NULL, NULL);
            letters[n] = '\0';
            n = recvfrom(sockfd, (char *)digits, MAX_BUFFER, 0, NULL, NULL);
            digits[n] = '\0';

            if (strcmp(letters, "Error: Invalid password") == 0) {
                printf("Server: %s\n", letters);
            } else {
                printf("Server: Doi mat khau thanh cong!\n");
                printf("Chu cai: %s\n", letters);
                printf("So: %s\n", digits);
            }
        } else {
            printf("Lenh khong hop le. Thu lai.\n");
        }
    }

    close(sockfd);
    return 0;
}

