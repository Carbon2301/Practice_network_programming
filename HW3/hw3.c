#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <netdb.h>

#define FILENAME "nguoidung.txt"
#define HISTORYFILE "history.txt"

typedef struct User {
    char username[50];
    char password[50];
    int status; 
    char homepage[50];
    struct User* next;
} User;

char correct_code[] = "20225593";
User* head = NULL;
User* currentUser = NULL;
int isLoggedIn = 0;


void loadUsersFromFile() {
    FILE* file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Cannot open file");
        exit(1);
    }

    char line[200];
    while (fgets(line, sizeof(line), file)) {
        User* newUser = (User*)malloc(sizeof(User));
        sscanf(line, "%s %s %d %s", newUser->username, newUser->password, &newUser->status, newUser->homepage);
        newUser->next = head;
        head = newUser;
    }
    fclose(file);
}


void saveUsersToFile() {
    FILE* file = fopen(FILENAME, "w");
    if (file == NULL) {
        perror("Cannot open file");
        exit(1);
    }

    User* current = head;
    while (current != NULL) {
        fprintf(file, "%s %s %d %s\n", current->username, current->password, current->status, current->homepage);
        current = current->next;
    }
    fclose(file);
}


bool is_valid_ip_address(const char *ip_address) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip_address, &(sa.sin_addr)) != 0;
}

bool is_valid_domain_name_but_not_ip_address(const char *domain) {
    return (strchr(domain, '.') != NULL && !is_valid_ip_address(domain));
}


void get_domain_from_ip(const char* ip) {
    struct sockaddr_in sa;
    char host[1024];

    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &sa.sin_addr);

    if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), host, sizeof(host), NULL, 0, 0) == 0) {
        printf("Domain: %s\n", host);
    } else {
        printf("Unable to resolve domain name for IP address\n");
    }
}


void get_ip_from_domain(const char* domain) {
    struct addrinfo hints, *res;
    struct sockaddr_in *addr;
    char ip[INET_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(domain, NULL, &hints, &res) == 0) {
        addr = (struct sockaddr_in *) res->ai_addr;
        inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
        printf("IP address: %s\n", ip);
        freeaddrinfo(res);
    } else {
        printf("Unable to resolve IP for domain\n");
    }
}


void registerUser() {
    User* newUser = (User*)malloc(sizeof(User));
    if (newUser == NULL) {
        printf("Memory allocation failed\n");
        return;
    }

    printf("Enter username: ");
    fgets(newUser->username, sizeof(newUser->username), stdin);
    newUser->username[strcspn(newUser->username, "\n")] = 0;

    printf("Enter password: ");
    fgets(newUser->password, sizeof(newUser->password), stdin);
    newUser->password[strcspn(newUser->password, "\n")] = 0;

    printf("Enter homepage (IP or domain): ");
    fgets(newUser->homepage, sizeof(newUser->homepage), stdin);
    newUser->homepage[strcspn(newUser->homepage, "\n")] = 0;

    User* current = head;
    while (current != NULL) {
        if (strcmp(current->username, newUser->username) == 0) {
            printf("Username already exists\n");
            free(newUser);
            return;
        }
        current = current->next;
    }

    newUser->status = 2; 
    newUser->next = head;
    head = newUser;
    saveUsersToFile();
    printf("Registered successfully!\n");
}


void signIn() {
    if (isLoggedIn) {
        printf("You are already logged in.\n");
        return;
    }

    char username[50], password[50];
    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    User* current = head;
    int attempts = 0;
    
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            if (current->status == 0) {
                printf("Your account is blocked.\n");
                return;
            }

            while (attempts < 3) {
                printf("Enter password: ");
                fgets(password, sizeof(password), stdin);
                password[strcspn(password, "\n")] = 0;

                if (strcmp(password, current->password) == 0) {
                    printf("Welcome\n");
                    isLoggedIn = 1;
                    currentUser = current;

                    // Log login history
                    FILE* historyFile = fopen(HISTORYFILE, "a");
                    time_t now = time(NULL);
                    struct tm *t = localtime(&now);
                    fprintf(historyFile, "%s | %02d/%02d/%d | %02d:%02d:%02d\n", 
                            currentUser->username, 
                            t->tm_mday, 
                            t->tm_mon + 1, 
                            t->tm_year + 1900, 
                            t->tm_hour, 
                            t->tm_min, 
                            t->tm_sec);
                    fclose(historyFile);
                    return;
                } else {
                    attempts++;
                    printf("Wrong password, %d tries left\n", 3 - attempts);
                }

                if (attempts == 3) {
                    current->status = 0; // Block account after 3 incorrect attempts
                    saveUsersToFile();
                    printf("Your account is blocked after 3 failed login attempts.\n");
                    return;
                }
            }
            return;
        }
        current = current->next;
    }
    printf("Account does not exist.\n");
}


void changePassword() {
    if (!isLoggedIn) {
        printf("You are not logged in.\n");
        return;
    }

    char oldPassword[50], newPassword[50];

    printf("Enter your current password: ");
    fgets(oldPassword, sizeof(oldPassword), stdin);
    oldPassword[strcspn(oldPassword, "\n")] = 0;

    if (strcmp(currentUser->password, oldPassword) != 0) {
        printf("Incorrect current password.\n");
        return;
    }

    printf("Enter your new password: ");
    fgets(newPassword, sizeof(newPassword), stdin);
    newPassword[strcspn(newPassword, "\n")] = 0;

    strcpy(currentUser->password, newPassword);
    saveUsersToFile();
    printf("Password updated successfully.\n");
}


void UpdateAccountInfo_Active() {
    if (!isLoggedIn) {
        printf("You need to sign in first\n");
        return;
    }

    if (currentUser->status == 0) {
        printf("Your account is already blocked.\n");
        return;
    }
    
    if (currentUser->status == 1) {
        printf("Your account is already actived.\n");
        return;
    }

    char Code[50];
    printf("Enter the activation code: ");
    fgets(Code, sizeof(Code), stdin);
    Code[strcspn(Code, "\n")] = 0;

    if (strcmp(Code, correct_code) == 0) {
        currentUser->status = 1; 
        saveUsersToFile();
        printf("Account activated successfully.\n");
    } else {
        currentUser->status = 0; 
        saveUsersToFile();
        printf("Incorrect activation code. Your account is now blocked.\n");
    }
}


void resetPassword() {
    char username[50];
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    User* current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            char verificationCode[50];
            printf("Enter the verification code: ");
            fgets(verificationCode, sizeof(verificationCode), stdin);
            verificationCode[strcspn(verificationCode, "\n")] = 0;

            if (strcmp(verificationCode, correct_code) == 0) {
                char newPassword[50];
                printf("Enter your new password: ");
                fgets(newPassword, sizeof(newPassword), stdin);
                newPassword[strcspn(newPassword, "\n")] = 0;

                strcpy(current->password, newPassword);
                saveUsersToFile();
                printf("Password reset successfully.\n");
                return;
            } else {
                printf("Incorrect verification code.\n");
                return;
            }
        }
        current = current->next;
    }
    printf("User not found.\n");
}


void viewLoginHistory() {
    if (!isLoggedIn) {
        printf("You are not logged in.\n");
        return;
    }

    FILE* file = fopen(HISTORYFILE, "r");
    if (file == NULL) {
        printf("No login history found.\n");
        return;
    }

    char line[200];
    printf("Your login history:\n");
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, currentUser->username) != NULL) {
            printf("%s", line);
        }
    }
    fclose(file);
}


void HomepageWithDomainName() {
    if (!isLoggedIn) {
        printf("You are not logged in yet.\n");
        return;
    }

    if (is_valid_domain_name_but_not_ip_address(currentUser->homepage)) {
        printf("Homepage domain: %s\n", currentUser->homepage);
    } else if (is_valid_ip_address(currentUser->homepage)) {
        printf("Resolving domain from IP: %s\n", currentUser->homepage);
        get_domain_from_ip(currentUser->homepage);
    } else {
        printf("Invalid homepage address.\n");
    }
}


void HomepageWithIpAddress() {
    if (!isLoggedIn) {
        printf("You are not logged in yet.\n");
        return;
    }

    if (is_valid_ip_address(currentUser->homepage)) {
        printf("Homepage IP address: %s\n", currentUser->homepage);
    } else if (is_valid_domain_name_but_not_ip_address(currentUser->homepage)) {
        printf("Resolving IP from domain: %s\n", currentUser->homepage);
        get_ip_from_domain(currentUser->homepage); 
    } else {
        printf("Invalid homepage address.\n");
    }
}


int main() {
    loadUsersFromFile();
    char input[10];
    int choice;

    do {
        printf("\nUSER MANAGEMENT PROGRAM\n");
        printf("-----------------------------------\n");
        printf("1. Register\n");
        printf("2. Sign in\n");
        printf("3. Change password\n");
        printf("4. Update Account Info (Active account)\n");
        printf("5. Reset password\n");
        printf("6. View login history\n");
        printf("7. Homepage with domain name\n");
        printf("8. Homepage with IP address\n");
        printf("9. Sign out\n");
        printf("Your choice (1-9, other to quit): ");

        fgets(input, sizeof(input), stdin);
        if (sscanf(input, "%d", &choice) != 1 || choice < 1 || choice > 9) {
            printf("Goodbye!\n");
            break;
        }

        switch (choice) {
            case 1:
                registerUser();
                break;
            case 2:
                signIn();
                break;
            case 3:
                changePassword();
                break;
            case 4:
                UpdateAccountInfo_Active();
                break;
            case 5:
                resetPassword();
                break;
            case 6:
                viewLoginHistory();
                break;
            case 7:
                HomepageWithDomainName();
                break;
            case 8:
                HomepageWithIpAddress();
                break;
            case 9:
                isLoggedIn = 0;
                printf("You have logged out.\n");
                break;
        }
    } while (1);

    return 0;
}

