#include "budget.h"
#include <limits.h>

void showMainMenu(void)
{
    puts("\n========== MYPOCKET: STUDENT BUDGET MANAGEMENT ==========");
    puts("1. Register\n2. Login\n3. Exit");
}

void showDashboard(int userId)
{
    int index = findUserById(userId);
    if (index >= 0) printf("\nWelcome, %s (User ID: %d)\n", users[index].name, userId);
    showFinancialSummary(userId);
}

void user_menu(int userId)
{
    int choice;
    if (!isValidUser(userId)) { puts("Invalid user session."); return; }
    processRecurringExpenses(userId);
    for (;;) {
        int index = findUserById(userId);
        puts("\n=============== MYPOCKET DASHBOARD ===============");
        printf("Welcome, %s | User ID: %d | Balance: %.2f BDT\n", users[index].name, userId, users[index].balance);
        puts("1. Income management");
        puts("2. Budget management and alerts");
        puts("3. Add expense / view transactions");
        puts("4. Search transactions & tokenization");
        puts("5. Reminders and calendar");
        puts("6. Recurring / upcoming expenses");
        puts("7. Financial summary and monthly review");
        puts("8. Group events and expense sharing");
        puts("9. Financial planning");
        puts("10. Savings management");
        puts("11. Edit profile");
        puts("12. Logout");
        choice = readInt("Choose an option: ", 1, 12);
        switch (choice) {
            case 1: manageIncome(userId); break;
            case 2: manageBudget(userId); break;
            case 3: {
                int transactionChoice;
                puts("\n1. Add expense\n2. View all my transactions\n3. View expenses by category\n4. Edit any financial record\n5. Delete a financial record\n6. Back");
                transactionChoice = readInt("Choose: ", 1, 6);
                if (transactionChoice == 1) addExpense(userId);
                else if (transactionChoice == 2) viewTransactions(userId);
                else if (transactionChoice == 3) viewTransactionsByCategory(userId, promptCategory());
                else if (transactionChoice == 4) editFinancialTransaction(userId);
                else if (transactionChoice == 5) deleteFinancialTransaction(userId);
                break;
            }
            case 4: {
                int searchChoice;
                puts("\n1. Search my transactions\n2. Tokenization demo\n3. Back");
                searchChoice = readInt("Choose: ", 1, 3);
                if (searchChoice == 1) searchTransactions(userId);
                else if (searchChoice == 2) search_and_tokenization_demo();
                break;
            }
            case 5: reminder_dashboard(userId); break;
            case 6: {
                int recurringChoice;
                puts("\n1. Add recurring expense\n2. View recurring expenses\n3. View upcoming expenses\n4. Process due expenses now\n5. Edit recurring expense\n6. Delete recurring expense\n7. Back");
                recurringChoice = readInt("Choose: ", 1, 7);
                if (recurringChoice == 1) addRecurringExpense(userId);
                else if (recurringChoice == 2) viewRecurringExpenses(userId);
                else if (recurringChoice == 3) showUpcomingExpenses(userId);
                else if (recurringChoice == 4) processRecurringExpenses(userId);
                else if (recurringChoice == 5) editRecurringExpense(userId);
                else if (recurringChoice == 6) deleteRecurringExpense(userId);
                break;
            }
            case 7: {
                int reportChoice;
                puts("\n1. Financial summary\n2. Sector-wise expenses\n3. Monthly review (month-to-month comparison)\n4. Export transactions to CSV\n5. Back");
                reportChoice = readInt("Choose: ", 1, 5);
                if (reportChoice == 1) showFinancialSummary(userId);
                else if (reportChoice == 2) showSectorWiseExpenses(userId);
                else if (reportChoice == 3) showMonthlyReview(userId);
                else if (reportChoice == 4) exportTransactionsCsv(userId);
                break;
            }
            case 8: event_dashboard(userId); break;
            case 9: showFinancialPlanning(userId); break;
            case 10: savingsManagement(userId); break;
            case 11: editProfile(userId); break;
            case 12: puts("Logging out..."); return;
            default: puts("Invalid choice."); break;
        }
    }
}

int main(int argc, char *argv[])
{
    int choice, userId;
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            puts("MyPocket - Student Budget Management System\nUsage: mypocket [--help | --version | --stats]");
            puts("Run without arguments to open the interactive menu.");
            return 0;
        }
        if (strcmp(argv[1], "--version") == 0) { puts("MyPocket 1.0 (C11 lab project)"); return 0; }
        if (strcmp(argv[1], "--stats") == 0) {
            loadAllData();
            printf("Saved users: %d\nTransactions: %d\nBudgets: %d\nEvents: %d\nReminders: %d\nRecurring expenses: %d\n",
                   userCount, transactionCount, budgetCount, eventCount, reminderCount, recurringCount);
            return 0;
        }
        fprintf(stderr, "Unknown argument: %s\nUse --help for usage.\n", argv[1]);
        return 1;
    }

    loadAllData();
    puts("Welcome to MyPocket. Your local data is loaded automatically.");
    for (;;) {
        showMainMenu();
        choice = readInt("Enter your choice: ", 1, 3);
        if (choice == INT_MIN) { puts("\nInput closed. Saving data and exiting."); saveAllData(); return 0; }
        switch (choice) {
            case 1:
                userId = registerUser();
                if (userId >= 0) user_menu(userId);
                break;
            case 2:
                userId = loginUser();
                if (userId >= 0) user_menu(userId);
                break;
            case 3:
                saveAllData();
                puts("Data saved. Goodbye!");
                return 0;
            default: break;
        }
    }
}
