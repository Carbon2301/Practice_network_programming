#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// C?u trúc c?a tài kho?n
typedef struct usr {
    char name[20];
    char pass[20];
    int status; // 0: blocked, 1: active
} acc;

typedef acc ListElementType;

// C?u trúc c?a node trong danh sách liên k?t
struct LISTNODE {
    ListElementType inf;
    struct LISTNODE *next;
};

typedef struct LISTNODE LISTNODE;

typedef struct list {
    LISTNODE *root;
    LISTNODE *cur;
} LIST;

// Danh sách l?ch s? dang nh?p
typedef struct history {
    char name[20];
    char action[20]; // login or logout
} History;

// Kh?i t?o danh sách l?ch s?
LIST *loginHistory;

// CÁC THAO TÁC TRÊN DSLK
LIST* newList(); // Kh?i t?o danh sách liên k?t m?i
void insertAfterCurrent(LIST*, ListElementType); // Thêm m?t ph?n t? vào sau current
LISTNODE* searchList(LIST*, char*); // Tìm ki?m trong danh sách theo tên tài kho?n
ListElementType registerAcc(LIST*); // Ðang ký tài kho?n m?i
void deleteNode(LIST*, char*); // Xóa tài kho?n
void changePassword(LIST*); // Ð?i m?t kh?u
void updateInfo(LIST*); // C?p nh?t thông tin tài kho?n
void recoverPassword(LIST*); // Khôi ph?c m?t kh?u
void displayLoginHistory(); // Hi?n th? l?ch s? dang nh?p
void logIn(LIST*, LIST*, int*); // Ðang nh?p
void logOut(LIST*, LIST*); // Ðang xu?t
void searchUser(LIST*); // Tìm ki?m tài kho?n
int readFile(LIST*, char*); // Ð?c d? li?u t? file
void saveFile(LIST*, char*); // Luu d? li?u vào file

void menuM() {
    printf("\nUSER MANAGEMENT PROGRAM\n");
    printf("---------------------------------------------\n");
    printf("\t1. Register\n");
    printf("\t2. Sign in\n");
    printf("\t3. Change Password\n");
    printf("\t4. Update Information\n");
    printf("\t5. Recover Password\n");
    printf("\t6. Search User\n");
    printf("\t7. View Login History\n");
    printf("\t8. Sign Out\n");
    printf("Your choice (1-8, other to quit): ");
}

LIST* newList() {
    LIST *list = malloc(sizeof(LIST));
    list->root = NULL;
    list->cur = list->root;
    return list;
}

LISTNODE* makeNewNode(ListElementType inf) {
    LISTNODE* new = (LISTNODE*) malloc(sizeof(LISTNODE));
    new->inf = inf;
    new->next = NULL;
    return new;
}

void insertAfterCurrent(LIST *list, ListElementType e) {
    LISTNODE *new = makeNewNode(e);
    if (list->root == NULL) {
        list->root = new;
        list->cur = list->root;
    } else {
        new->next = list->cur->next;
        list->cur->next = new;
        list->cur = new;
    }
}

// Tìm ki?m tài kho?n
LISTNODE* searchList(LIST* list, char X[]) {
    LISTNODE *p;
    for (p = list->root; p != NULL; p = p->next) {
        if (strcasecmp(p->inf.name, X) == 0) {
            return p;
        }
    }
    return NULL;
}

// Ðang ký tài kho?n m?i
ListElementType registerAcc(LIST* list) {
    ListElementType temp;
    LISTNODE* p;

    printf("Username: ");
    scanf("%s", temp.name);
    p = searchList(list, temp.name);
    if (p != NULL) {
        printf("Account already exists!\n");
    } else {
        printf("Password: ");
        scanf("%s", temp.pass);
        temp.status = 1;
        printf("Successful registration\n");
        return temp;
    }
    return temp;
}

// Ðang nh?p
void logIn(LIST *list, LIST *listLogin, int *k) {
    char tempUser[20];
    char tempPass[20];
    LISTNODE* tempnode;

    do {
        printf("Username: ");
        scanf("%s", tempUser);
        tempnode = searchList(list, tempUser);
        if (tempnode == NULL) {
            printf("Cannot find account\n");
        } else if (tempnode->inf.status == 0) {
            printf("Account is blocked\n");
        }
    } while (tempnode == NULL || tempnode->inf.status == 0);

    do {
        printf("Password: ");
        scanf("%s", tempPass);
        if (strcmp(tempPass, tempnode->inf.pass) != 0) {
            printf("Password is incorrect\n");
            (*k)++;
        } else {
            *k = 0;
        }
    } while (strcasecmp(tempPass, tempnode->inf.pass) != 0 && *k < 3);

    if (*k >= 3) {
        tempnode->inf.status = 0;
        printf("Account is blocked after 3 failed attempts\n");
    } else {
        insertAfterCurrent(listLogin, tempnode->inf);
        printf("Hello %s\n", tempnode->inf.name);
        
        // Luu l?ch s? dang nh?p
        History hist;
        strcpy(hist.name, tempnode->inf.name);
        strcpy(hist.action, "login");
    }
}

// Ðang xu?t
void logOut(LIST *list, LIST *listLogin) {
    char tempUser[20];
    LISTNODE* tempnode;

    printf("Username: ");
    scanf("%s", tempUser);
    tempnode = searchList(listLogin, tempUser);
    if (tempnode == NULL) {
        printf("Account is not signed in\n");
    } else {
        printf("Goodbye %s\n", tempUser);
        deleteNode(listLogin, tempUser);

        // Luu l?ch s? dang xu?t
        History hist;
        strcpy(hist.name, tempUser);
        strcpy(hist.action, "logout");
    }
}

// Ð?i m?t kh?u
void changePassword(LIST *list) {
    char tempUser[20], oldPass[20], newPass[20];
    LISTNODE* tempnode;

    printf("Username: ");
    scanf("%s", tempUser);
    tempnode = searchList(list, tempUser);
    if (tempnode == NULL) {
        printf("Account not found\n");
        return;
    }

    printf("Old password: ");
    scanf("%s", oldPass);
    if (strcmp(oldPass, tempnode->inf.pass) != 0) {
        printf("Incorrect password\n");
        return;
    }

    printf("New password: ");
    scanf("%s", newPass);
    strcpy(tempnode->inf.pass, newPass);
    printf("Password changed successfully\n");
}

// C?p nh?t thông tin tài kho?n
void updateInfo(LIST *list) {
    char tempUser[20];
    LISTNODE* tempnode;

    printf("Username: ");
    scanf("%s", tempUser);
    tempnode = searchList(list, tempUser);
    if (tempnode == NULL) {
        printf("Account not found\n");
        return;
    }

    printf("Updating account for %s\n", tempUser);
    printf("New password: ");
    scanf("%s", tempnode->inf.pass);
    printf("Account information updated successfully\n");
}

// Khôi ph?c m?t kh?u
void recoverPassword(LIST *list) {
    char tempUser[20], newPass[20];
    LISTNODE* tempnode;

    printf("Username: ");
    scanf("%s", tempUser);
    tempnode = searchList(list, tempUser);
    if (tempnode == NULL) {
        printf("Account not found\n");
        return;
    }

    printf("New password: ");
    scanf("%s", newPass);
    strcpy(tempnode->inf.pass, newPass);
    printf("Password recovered successfully\n");
}

// Hi?n th? l?ch s? dang nh?p
void displayLoginHistory() {
    // Hi?n th? thông tin l?ch s? t? danh sách loginHistory
}

int main() {
    int n, k = 0;
    LIST *list = newList();
    LIST *listLogin = newList();

    if (readFile(list, "account.txt")) {
        do {
            menuM();
            scanf("%d", &n);
            switch (n) {
                case 1: 
                    insertAfterCurrent(list, registerAcc(list)); 
                    break;
                case 2: 
                    logIn(list, listLogin, &k); 
                    break;
                case 3: 
                    changePassword(list); 
                    break;
                case 4: 
                    updateInfo(list); 
                    break;
                case 5: 
                    recoverPassword(list); 
                    break;
                case 6: 
                    searchUser(list); 
                    break;
                case 7: 
                    displayLoginHistory(); 
                    break;
                case 8: 
                    logOut(list, listLogin); 
                    break;
                default: 
                    break;
            }
        } while (n >= 1 && n <= 8);
        
        saveFile(list, "account.txt");
    }
    
    return 0;
}

