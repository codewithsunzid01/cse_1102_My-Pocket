#include "budget.h"
#include <ctype.h>

int findUserByEmail(const char *email)
{
    int i;
    for (i = 0; i < userCount; ++i) {
        if (strcmp(users[i].email, email) == 0) return i;
    }
    return -1;
}

int findUserById(int userId)
{
    int i;
    for (i = 0; i < userCount; ++i) {
        if (users[i].userId == userId) return i;
    }
    return -1;
}

int authenticateUser(const char *email, const char *password)
{
    int index = findUserByEmail(email);
    if (index < 0) return -1;
    return strcmp(users[index].password, password) == 0 ? users[index].userId : -1;
}

int isStrongPassword(const char *password)
{
    int upper = 0, lower = 0, digit = 0, special = 0;
    size_t i, length = strlen(password);
    if (length < 6U || length >= MAX_PASSWORD_LEN) return 0;
    for (i = 0U; i < length; ++i) {
        unsigned char ch = (unsigned char)password[i];
        if (isupper(ch)) upper = 1;
        else if (islower(ch)) lower = 1;
        else if (isdigit(ch)) digit = 1;
        else special = 1;
    }
    return upper && lower && digit && special;
}

static int nextUserId(void)
{
    int i, maximum = 0;
    for (i = 0; i < userCount; ++i) if (users[i].userId > maximum) maximum = users[i].userId;
    return maximum + 1;
}

static void copyInput(char *destination, size_t capacity, const char *source)
{
    if (capacity == 0U) return;
    (void)snprintf(destination, capacity, "%s", source);
}

int registerUser(void)
{
    char *name, *email, *password, *extra;
    int id, extraChoice;
    User *user;

    if (userCount >= MAX_USERS) {
        puts("User limit reached. Cannot register more users.");
        return -1;
    }
    puts("\n========== USER REGISTRATION ==========");
    name = readLineDynamic("Enter your full name: ");
    if (name == NULL || name[0] == '\0') { free(name); puts("Name is required."); return -1; }
    email = readLineDynamic("Enter your email: ");
    if (email == NULL || email[0] == '\0' || strlen(email) >= MAX_EMAIL_LEN) {
        free(name); free(email); puts("A valid email under 100 characters is required."); return -1;
    }
    if (strchr(email, '@') == NULL || strchr(email, '.') == NULL || findUserByEmail(email) >= 0) {
        puts("Email is invalid or already registered."); free(name); free(email); return -1;
    }

    for (;;) {
        password = readLineDynamic("Create password (6+ chars, upper/lower/digit/special): ");
        if (password == NULL) { free(name); free(email); return -1; }
        if (isStrongPassword(password)) break;
        puts("Weak password. Example format: Student@123");
        free(password);
    }
    if (strlen(name) >= MAX_NAME_LEN) {
        puts("Name is too long (maximum 49 characters).");
        free(name); free(email); free(password); return -1;
    }

    id = nextUserId();
    user = &users[userCount];
    memset(user, 0, sizeof(*user));
    user->userId = id;
    copyInput(user->name, sizeof(user->name), name);
    copyInput(user->email, sizeof(user->email), email);
    copyInput(user->password, sizeof(user->password), password);
    user->monthly_income = 0.0;
    user->monthly_budget = 0.0;
    user->savings_goal = 0.0;
    user->total_expense = 0.0;
    user->balance = 0.0;

    puts("\nOptional profile information:");
    puts("1. Phone number  2. Student ID  3. Skip");
    extraChoice = readInt("Choose: ", 1, 3);
    if (extraChoice == 1 || extraChoice == 2) {
        extra = readLineDynamic(extraChoice == 1 ? "Enter phone number: " : "Enter student ID: ");
        if (extra != NULL) {
            if (extraChoice == 1) copyInput(user->extra.phone, sizeof(user->extra.phone), extra);
            else copyInput(user->extra.studentId, sizeof(user->extra.studentId), extra);
        }
        free(extra);
    }

    ++userCount;
    saveAllData();
    printf("\nRegistration successful! Your User ID is %d.\n", id);
    free(name); free(email); free(password);
    return id;
}

int loginUser(void)
{
    char *email, *password;
    int userId;
    puts("\n============= USER LOGIN =============");
    email = readLineDynamic("Enter your email: ");
    if (email == NULL) return -1;
    password = readLineDynamic("Enter your password: ");
    if (password == NULL) { free(email); return -1; }
    userId = authenticateUser(email, password);
    free(email); free(password);
    if (userId < 0) {
        puts("Invalid email or password.");
        return -1;
    }
    printf("Login successful. Welcome, %s!\n", users[findUserById(userId)].name);
    return userId;
}


void editProfile(int userId)
{
    int index = findUserById(userId);
    char *name = NULL, *email = NULL, *password = NULL;
    if (index < 0) { puts("User not found."); return; }
    printf("\n===== EDIT PROFILE (User ID %d) =====\n", userId);
    name = readLineDynamic("New name (Enter keeps current): ");
    if (name == NULL) return;
    email = readLineDynamic("New email (Enter keeps current): ");
    if (email == NULL) { free(name); return; }
    password = readLineDynamic("New password (Enter keeps current): ");
    if (password == NULL) { free(name); free(email); return; }
    if (name[0] != '\0' && strlen(name) >= MAX_NAME_LEN) {
        puts("Name is too long; no changes made."); free(name); free(email); free(password); return;
    }
    if (email[0] != '\0') {
        int existing = findUserByEmail(email);
        if (strchr(email, '@') == NULL || strchr(email, '.') == NULL || (existing >= 0 && existing != index)) {
            puts("Email is invalid or already used by another account; no changes made.");
            free(name); free(email); free(password); return;
        }
    }
    if (password[0] != '\0' && !isStrongPassword(password)) {
        puts("New password must be 6+ characters with upper/lowercase, digit, and special character; no changes made.");
        free(name); free(email); free(password); return;
    }
    if (name[0] != '\0') (void)snprintf(users[index].name, sizeof(users[index].name), "%s", name);
    if (email[0] != '\0') (void)snprintf(users[index].email, sizeof(users[index].email), "%s", email);
    if (password[0] != '\0') (void)snprintf(users[index].password, sizeof(users[index].password), "%s", password);
    saveAllData();
    puts("Profile updated successfully.");
    free(name); free(email); free(password);
}
