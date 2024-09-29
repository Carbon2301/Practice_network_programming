#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define FILENAME "account.txt"
#define HISTORYFILE "history.txt"

typedef struct User {
    char username[50];
    char password[50];
    int status; 
    char homepage[100]; // Them truong homepage
    struct User* next;
} User;

User* head = NULL; 			
User* currentUser = NULL;   
int isLoggedIn = 0;        

// Doc file tai khoan vao danh sach lien ket
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

// Luu danh sach nguoi dung vao file
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

// Dang ky nguoi dung moi
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

    printf("Enter homepage (domain or IP address): ");
    fgets(newUser->homepage, sizeof(newUser->homepage), stdin);
    newUser->homepage[strcspn(newUser->homepage, "\n")] = 0; 

    // Kiem tra xem username da ton tai chua
    User* current = head;
    while (current != NULL) {
        if (strcmp(current->username, newUser->username) == 0) {
            printf("Username already exists\n");
            free(newUser);
            return;
        }
        current = current->next;
    }

    newUser->status = 1; 
    newUser->next = head;
    head = newUser;
    saveUsersToFile();
    printf("Registered successfully.\n");
}

// Dang nhap nguoi dung
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

                if (strcmp(current->password, password) == 0) {
                    printf("Welcome\n");
                    isLoggedIn = 1;
                    currentUser = current;

                    // Luu lich su dang nhap
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
                    return; // Dang nhap thanh cong, thoat ham
                } else {
                    attempts++;
                    printf("Wrong password, %d tries left\n", 3 - attempts);
                }

                if (attempts == 3) {
                    current->status = 0; // Khoa tai khoan
                    saveUsersToFile();
                    printf("Your account is blocked.\n");
                    return;
                }
            }
            return; // Thoat neu dang nhap khong thanh cong
        }
        current = current->next;
    }
    printf("Account does not exist.\n");
}

// Hien thi homepage (ten mien)
void viewHomepageDomain() {
    if (!isLoggedIn) {
        printf("You are not logged in yet.\n");
        return;
    }
    printf("Your homepage (domain name): %s\n", currentUser->homepage);
}

// Hien thi homepage (dia chi IP)
void viewHomepageIP() {
    if (!isLoggedIn) {
        printf("You are not logged in yet.\n");
        return;
    }
    printf("Your homepage (IP address): %s\n", currentUser->homepage);
}

// Dang xuat nguoi dung
void signOut() {
    if (!isLoggedIn) {
        printf("You are not logged in yet.\n");
        return;
    }
    printf("Goodbye %s!\n", currentUser->username);
    currentUser = NULL;
    isLoggedIn = 0;
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
        printf("4. Update account info\n");
        printf("5. Reset password\n");
        printf("6. View login history\n");
        printf("7. Homepage (domain)\n");
        printf("8. Homepage (IP address)\n");
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
                updateAccountInfo();
                break;
            case 5:
                resetPassword();
                break;
            case 6:
                viewLoginHistory();
                break;
            case 7:
                viewHomepageDomain();
                break;
            case 8:
                viewHomepageIP();
                break;
            case 9:
                signOut();
                break;
        }

    } while (1); 

    return 0;
}

