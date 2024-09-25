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
    char email[50];
    char phone[30];
    int status; 
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
        sscanf(line, "%s %s %s %s %d", newUser->username, newUser->password, newUser->email, newUser->phone, &newUser->status);
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
        fprintf(file, "%s %s %s %s %d\n", current->username, current->password, current->email, current->phone, current->status);
        current = current->next;
    }
    fclose(file);
}
///////////////////////////////////////////////////////////////////////////////////////////////
//Ham kiem tra xem lieu so dien thoai co chua ki tu hay khong
int isValidPhoneNumber(const char* phone) {
    for (int i = 0; phone[i] != '\0'; i++) {
        if (!isdigit(phone[i])) {
            return 0; 
        }
    }
    return 1; 
}

//Ham kiem tra xem lieu email co chua dau cach hay khong
int isEmailValid(const char* email) {
    while (*email) {
        if (isspace(*email)) {
            return 0; // 
        }
        email++;
    }
    return 1; // 
}


void registerUser() {
    User* newUser = (User*)malloc(sizeof(User));
    if (newUser == NULL) {
        printf("Memory allocation failed\n");
        return;
    }

  
    printf("Enter username: ");
    fgets(newUser->username, sizeof(newUser->username), stdin);
    newUser->username[strcspn(newUser->username, "\n")] = 0; // loai bo ki tu newline


    printf("Enter password: ");
    fgets(newUser->password, sizeof(newUser->password), stdin);
    newUser->password[strcspn(newUser->password, "\n")] = 0; 

    while (1) {
        printf("Enter email: ");
        fgets(newUser->email, sizeof(newUser->email), stdin);
        newUser->email[strcspn(newUser->email, "\n")] = 0; 

        if (strlen(newUser->email) > 40) {
            printf("Email is too long. Please enter again.\n");
        } else if (!isEmailValid(newUser->email)) {
            printf("Email must not contain spaces. Please enter again.\n");
        } else {
            break; 
        }
    }
 

    //Kiem tra tinh hop le so dien thoai
    while (1) {
        printf("Enter phone: ");
        fgets(newUser->phone, sizeof(newUser->phone), stdin);
        newUser->phone[strcspn(newUser->phone, "\n")] = 0; 

        // Kiem tra do dai va tinh hop le
        if (strlen(newUser->phone) > 12) {
            printf("Phone number is too long. Please enter again.\n");
        } else if (!isValidPhoneNumber(newUser->phone)) {
            printf("Phone number must contain only digits. Please enter again.\n");
        } else {
            break; 
        }
    }

    
    User* current = head;
    while (current != NULL) {
        if (strcmp(current->username, newUser->username) == 0) {
            printf("Username already exists\n");
            free(newUser);
            return;
        }
        if (strcmp(current->email, newUser->email) == 0) {
            printf("Email already exists\n");
            free(newUser);
            return;
        }
        if (strcmp(current->phone, newUser->phone) == 0) {
            printf("Phone already exists\n");
            free(newUser);
            return;
        }
        current = current->next;
    }

    newUser->status = 1; // kiach hoat tai khoan
    newUser->next = head;
    head = newUser;
    saveUsersToFile();
    printf("Registered successfully.\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////
void signIn() {
    if (isLoggedIn) {
        printf("You are already logged in.\n");
        return;
    }

    char username[50], password[50];
    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0; // Loai bo ki tu newline

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
                password[strcspn(password, "\n")] = 0; // Loai bo ki tu newline

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

////////////////////////////////////////////////////////////////////////////////////////////
// Doi mat khau
void changePassword() {
    if (!isLoggedIn) {
        printf("You are not logged in yet.\n");
        return;
    }

    char oldPassword[50];
    char newPassword[50];

    printf("Enter old password: ");
    fgets(oldPassword, sizeof(oldPassword), stdin);
    oldPassword[strcspn(oldPassword, "\n")] = 0; // Loai bo ki tu newline

    // Kiem tra mat khau cu
    if (strcmp(currentUser->password, oldPassword) == 0) {
   
        printf("Enter new password: ");
        fgets(newPassword, sizeof(newPassword), stdin);
        newPassword[strcspn(newPassword, "\n")] = 0; 

        // Kiem tra xem mat khau moi co trung voi mat khau cu
        while (strcmp(newPassword, oldPassword) == 0) {
            printf("\nThe new password matches the old password. ");
            printf("\nEnter new password: ");
            fgets(newPassword, sizeof(newPassword), stdin);
            newPassword[strcspn(newPassword, "\n")] = 0;
        }

        //Cap nhat mat khau
        strcpy(currentUser->password, newPassword);

        saveUsersToFile(); // Ghi l?i thông tin m?i
        printf("Password changed successfully.\n");
    } else {
        printf("The old password is incorrect.\n");
    }
}


//////////////////////////////////////////////////////////////////////////////////////
void updateAccountInfo() {
    if (!isLoggedIn) {
        printf("You are not logged in yet.\n");
        return;
    }

   
    while (1) {
        printf("Enter new email: ");
        char newEmail[50];
        fgets(newEmail, sizeof(newEmail), stdin);
        newEmail[strcspn(newEmail, "\n")] = 0; // 

        if (strlen(newEmail) > 40) {
            printf("Email is too long. Please enter again.\n");
        } else if (!isEmailValid(newEmail)) {
            printf("Email must not contain spaces. Please enter again.\n");
        } else {
            strcpy(currentUser->email, newEmail); 
            break; 
        }
    }

  
    while (1) {
        printf("Enter new phone: ");
        char newPhone[30];
        fgets(newPhone, sizeof(newPhone), stdin);
        newPhone[strcspn(newPhone, "\n")] = 0; 

        if (strlen(newPhone) > 12) {
            printf("Phone number is too long. Please enter again.\n");
        } else if (!isValidPhoneNumber(newPhone)) {
            printf("Phone number must contain only digits. Please enter again.\n");
        } else {
            strcpy(currentUser->phone, newPhone); 
            break; // 
        }
    }

    
    saveUsersToFile();
    currentUser->status = 1;
    printf("Updated information successfully.\n");
}
////////////////////////////////////////////////////////////////////////////////////////////
// Khoi phuc mat khau
void resetPassword() {
    char username[50];
    char verificationCode[50];
    
    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0; // Loai bo ki tu newline

    printf("Enter the verification code: ");
    fgets(verificationCode, sizeof(verificationCode), stdin);
    verificationCode[strcspn(verificationCode, "\n")] = 0;

    if (strcmp(verificationCode, "20225593") == 0) {
        User* current = head;
        while (current != NULL) {
            if (strcmp(current->username, username) == 0) {
                printf("Enter new password: ");
                char newPassword[50];
                fgets(newPassword, sizeof(newPassword), stdin);
                newPassword[strcspn(newPassword, "\n")] = 0; 
                
                // Cap nhat mat khau
                strcpy(current->password, newPassword);
                saveUsersToFile();
                printf("Password recovery successful.\n");
                return; 
            }
            current = current->next;
        }
        printf("Account does not exist.\n");
    } else {
        printf("Verification code is incorrect.\n");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Xem lich su dang nhap
void viewLoginHistory() {
    if (!isLoggedIn) {
        printf("You are not logged in yet.\n");
        return;
    }

    FILE* file = fopen(HISTORYFILE, "r");
    if (file == NULL) {
        printf("Cannot open file history.txt.\n");
        return;
    }

    char line[100];
    printf("Your login history:\n");
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, currentUser->username) != NULL) {
            printf("%s", line);
        }
    }
    fclose(file);
}
//////////////////////////////////////////////////////////////////////////////////////////
// Dang xuat
void signOut() {
    if (!isLoggedIn) {
        printf("You are not logged in yet.\n");
        return;
    }
    printf("Goodbye %s!\n", currentUser->username);
    currentUser = NULL;
    isLoggedIn = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
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
        printf("7. Sign out\n");
        printf("Your choice (1-7, other to quit): ");
        
        fgets(input, sizeof(input), stdin); 

        if (sscanf(input, "%d", &choice) != 1 || choice < 1 || choice > 7) {
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
                signOut();
                break;
        }

    } while (1); 

    return 0;
}
