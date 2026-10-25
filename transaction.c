#include "budget.h"
#include <math.h>
#include <limits.h>

static int nextTransactionId(void)
{
    int maxId = 0;
    for (int i = 0; i < transactionCount; ++i) if (transactions[i].transactionId > maxId) maxId = transactions[i].transactionId;
    return maxId + 1;
}

void addTransaction(int userId, TransactionType type, CategoryType category, double amount, const char *date, const char *description)
{
    Transaction *transaction;
    int userIndex = findUserById(userId);
    char currentDate[MAX_DATE_LEN];
    char month[MAX_MONTH_LEN];

    if (userIndex < 0) { puts("Cannot add transaction: user not found."); return; }
    if (!isfinite(amount) || amount <= 0.0) { puts("Cannot add a non-positive transaction."); return; }
    if (transactionCount >= MAX_TRANSACTIONS) { puts("Transaction storage is full; no transaction was recorded."); return; }

    transaction = &transactions[transactionCount];
    memset(transaction, 0, sizeof(*transaction));
    transaction->transactionId = nextTransactionId();
    transaction->userId = userId;
    transaction->type = type;
    transaction->category = category;
    transaction->amount = amount;
    getCurrentDate(currentDate);
    if (date != NULL && isValidDate(date)) (void)snprintf(transaction->date, sizeof(transaction->date), "%s", date);
    else (void)snprintf(transaction->date, sizeof(transaction->date), "%s", currentDate);
    if (description != NULL) (void)snprintf(transaction->description, sizeof(transaction->description), "%s", description);
    /* Store the type-specific note in the corresponding union member. */
    if (type == INCOME) {
        (void)snprintf(transaction->details.incomeSource, sizeof(transaction->details.incomeSource), "%s",
                       description != NULL ? description : "Income");
    } else if (type == EXPENSE) {
        (void)snprintf(transaction->details.expenseNote, sizeof(transaction->details.expenseNote), "%s",
                       description != NULL ? description : "Expense");
    } else {
        (void)snprintf(transaction->details.internalNote, sizeof(transaction->details.internalNote), "%s",
                       description != NULL ? description : "Internal transfer");
    }
    ++transactionCount;

    users[userIndex].monthly_income = calculateTotalIncome(userId);
    users[userIndex].total_expense = calculateTotalExpense(userId);
    (void)snprintf(month, sizeof(month), "%.7s", currentDate);
    for (int i = 0; i < budgetCount; ++i) {
        if (budgets[i].userId == userId && strcmp(budgets[i].month, month) == 0) {
            budgets[i].spent = users[userIndex].total_expense;
            budgets[i].remaining = budgets[i].amount - budgets[i].spent;
        }
    }
    saveAllData();
}

void addExpense(int userId)
{
    int index = findUserById(userId);
    CategoryType category;
    double amount;
    char *description;
    char date[MAX_DATE_LEN];
    if (index < 0) { puts("User not found."); return; }
    if (transactionCount >= MAX_TRANSACTIONS) { puts("Transaction storage is full."); return; }

    puts("\n========== ADD EXPENSE ==========");
    category = promptCategory();
    amount = readDouble("Enter expense amount (BDT): ", 0.01, 1000000000000.0);
    description = readLineDynamic("Description (e.g. lunch, bus fare, books): ");
    if (description == NULL || description[0] == '\0') {
        free(description);
        puts("Description is required; expense cancelled.");
        return;
    }
    getCurrentDate(date);
    users[index].balance -= amount;
    addTransaction(userId, EXPENSE, category, amount, date, description);
    printf("Expense saved. Balance is now %.2f BDT.\n", users[index].balance);
    free(description);
    checkBudgetAlert(userId);
}


void editExpense(int userId)
{
    int selectedId, transactionIndex = -1, userIndex = findUserById(userId);
    double oldAmount, newAmount;
    CategoryType newCategory;
    char *newDateInput = NULL, *newDescription = NULL;
    char currentDate[MAX_DATE_LEN], currentMonth[MAX_MONTH_LEN];

    if (userIndex < 0) { puts("User not found."); return; }

    puts("\n========== EDIT EXPENSE ==========");
    puts("Your recorded expenses:");
    printf("%-5s %-12s %-14s %-12s %s\n", "ID", "Date", "Category", "Amount", "Description");
    for (int i = 0; i < transactionCount; ++i) {
        const Transaction *t = &transactions[i];
        if (t->userId == userId && t->type == EXPENSE) {
            printf("%-5d %-12s %-14s %10.2f  %s\n", t->transactionId, t->date,
                   categoryToString(t->category), t->amount, t->description);
        }
    }

    selectedId = readInt("Enter the ID of the expense to edit (0 to cancel): ", 0, 2147483647);
    if (selectedId == 0) { puts("Edit cancelled."); return; }

    for (int i = 0; i < transactionCount; ++i) {
        if (transactions[i].transactionId == selectedId &&
            transactions[i].userId == userId && transactions[i].type == EXPENSE) {
            transactionIndex = i;
            break;
        }
    }
    if (transactionIndex < 0) {
        puts("Expense ID not found in your account. No changes made.");
        return;
    }

    printf("Current amount: %.2f BDT\n", transactions[transactionIndex].amount);
    oldAmount = transactions[transactionIndex].amount;
    newAmount = readDouble("Enter corrected amount (BDT): ", 0.01, 1000000000000.0);
    if (!isfinite(newAmount)) { puts("Invalid amount. No changes made."); return; }
    newCategory = promptCategory();

    printf("Current date: %s (press Enter to keep it)\n", transactions[transactionIndex].date);
    newDateInput = readLineDynamic("Enter new date (YYYY-MM-DD) or press Enter to keep: ");
    if (newDateInput == NULL) return;
    if (newDateInput[0] != '\0' && !isValidDate(newDateInput)) {
        puts("Invalid date. No changes made.");
        free(newDateInput);
        return;
    }

    printf("Current description: %s\n", transactions[transactionIndex].description);
    newDescription = readLineDynamic("Enter new description or press Enter to keep: ");
    if (newDescription == NULL) { free(newDateInput); return; }
    /* Correct the live balance by reversing the old expense and applying the new one. */
    users[userIndex].balance += oldAmount - newAmount;
    transactions[transactionIndex].amount = newAmount;
    transactions[transactionIndex].category = newCategory;
    if (newDateInput[0] != '\0') {
        (void)snprintf(transactions[transactionIndex].date,
                       sizeof(transactions[transactionIndex].date), "%s", newDateInput);
    }
    if (newDescription[0] != '\0') {
        (void)snprintf(transactions[transactionIndex].description,
                       sizeof(transactions[transactionIndex].description), "%s", newDescription);
    }
    {
        char noteCopy[MAX_DESC_LEN];
        (void)snprintf(noteCopy, sizeof(noteCopy), "%s", transactions[transactionIndex].description);
        memcpy(transactions[transactionIndex].details.expenseNote, noteCopy, sizeof(noteCopy));
    }

    getCurrentDate(currentDate);
    (void)snprintf(currentMonth, sizeof(currentMonth), "%.7s", currentDate);
    users[userIndex].monthly_income = calculateTotalIncome(userId);
    users[userIndex].total_expense = calculateTotalExpense(userId);
    for (int i = 0; i < budgetCount; ++i) {
        if (budgets[i].userId == userId && strcmp(budgets[i].month, currentMonth) == 0) {
            budgets[i].spent = users[userIndex].total_expense;
            budgets[i].remaining = budgets[i].amount - budgets[i].spent;
        }
    }
    saveAllData();
    printf("Expense #%d updated successfully. New amount: %.2f BDT. Current balance: %.2f BDT.\n",
           selectedId, newAmount, users[userIndex].balance);
    free(newDateInput);
    free(newDescription);
    checkBudgetAlert(userId);
}

void viewTransactions(int userId)
{
    int found = 0;
    printf("\n========== TRANSACTION HISTORY ==========");
    printf("\n%-5s %-12s %-20s %-14s %-12s %s\n", "ID", "Date", "Type", "Category", "Amount", "Description");
    for (int i = 0; i < transactionCount; ++i) {
        const Transaction *t = &transactions[i];
        if (t->userId != userId) continue;
        char displayDate[64];
        formatDateDisplay(t->date, displayDate, sizeof(displayDate));
        printf("%-5d %-20s %-20s %-14s %10.2f  %s\n", t->transactionId, displayDate,
               transactionTypeToString(t->type), categoryToString(t->category), t->amount, t->description);
        found = 1;
    }
    if (!found) puts("\nNo transactions recorded for this account yet.");
}

void viewTransactionsByCategory(int userId, CategoryType category)
{
    int found = 0;
    printf("\nExpenses in category: %s\n", categoryToString(category));
    for (int i = 0; i < transactionCount; ++i) {
        const Transaction *t = &transactions[i];
        if (t->userId == userId && t->type == EXPENSE && t->category == category) {
            char displayDate[64];
            formatDateDisplay(t->date, displayDate, sizeof(displayDate));
            printf("#%d | %s | %.2f BDT | %s\n", t->transactionId, displayDate, t->amount, t->description);
            found = 1;
        }
    }
    if (!found) puts("No expense found in this category.");
}



static double rawSavingsBalance(int userId)
{
    double balance = 0.0;
    for (int i = 0; i < transactionCount; ++i) {
        if (transactions[i].userId != userId) continue;
        if (transactions[i].type == SAVINGS_DEPOSIT) balance += transactions[i].amount;
        else if (transactions[i].type == SAVINGS_WITHDRAWAL) balance -= transactions[i].amount;
    }
    return balance;
}

static int isEventLinkedExpense(const Transaction *t)
{
    if (t == NULL || t->type != EXPENSE) return 0;
    for (int i = 0; i < eventCount; ++i) {
        for (int j = 0; j < events[i].expenseCount; ++j) {
            const EventExpense *e = &events[i].expenses[j];
            if (e->paidBy == t->userId && fabs(e->totalAmount - t->amount) < 0.005 &&
                strcmp(e->date, t->date) == 0 && strcmp(e->title, t->description) == 0) return 1;
        }
    }
    return 0;
}

static void refreshUserTotals(int userId)
{
    int ui = findUserById(userId);
    char today[MAX_DATE_LEN], month[MAX_MONTH_LEN];
    if (ui < 0) return;
    users[ui].monthly_income = calculateTotalIncome(userId);
    users[ui].total_expense = calculateTotalExpense(userId);
    getCurrentDate(today);
    (void)snprintf(month, sizeof(month), "%.7s", today);
    for (int i = 0; i < budgetCount; ++i) {
        if (budgets[i].userId == userId && strcmp(budgets[i].month, month) == 0) {
            budgets[i].spent = calculateTotalExpense(userId);
            budgets[i].remaining = budgets[i].amount - budgets[i].spent;
        }
    }
}

void editFinancialTransaction(int userId)
{
    int id, ti = -1, ui = findUserById(userId);
    double oldAmount, newAmount;
    char *dateInput = NULL, *description = NULL;
    if (ui < 0) { puts("User not found."); return; }
    puts("\n===== EDIT FINANCIAL RECORD =====");
    puts("Editable records (income, expense, savings deposit/withdrawal):");
    for (int i = 0; i < transactionCount; ++i) {
        Transaction *t = &transactions[i];
        if (t->userId == userId && t->type != INTERNAL_TRANSACTION) {
            char d[64]; formatDateDisplay(t->date, d, sizeof(d));
            printf("ID %d | %-20s | %10.2f BDT | %s\n", t->transactionId,
                   transactionTypeToString(t->type), t->amount, d);
        }
    }
    id = readInt("Transaction ID to edit (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    for (int i = 0; i < transactionCount; ++i)
        if (transactions[i].transactionId == id && transactions[i].userId == userId) { ti = i; break; }
    if (ti < 0 || transactions[ti].type == INTERNAL_TRANSACTION) { puts("Editable transaction not found. Internal transfers must be corrected through their event/transfer record."); return; }
    if (isEventLinkedExpense(&transactions[ti])) { puts("This expense belongs to a group event. Edit it through Group Events to keep settlement consistent."); return; }
    oldAmount = transactions[ti].amount;
    printf("Current amount: %.2f BDT\n", oldAmount);
    newAmount = readDouble("Correct amount (BDT): ", 0.01, 1000000000000.0);
    if (!isfinite(newAmount)) { puts("Invalid amount."); return; }
    if (transactions[ti].type == INCOME) {
        users[ui].balance += newAmount - oldAmount;
    } else if (transactions[ti].type == EXPENSE) {
        users[ui].balance += oldAmount - newAmount;
    } else if (transactions[ti].type == SAVINGS_DEPOSIT) {
        double increase = newAmount - oldAmount;
        if (increase > users[ui].balance + 0.000001) { puts("Insufficient available balance for the increased savings deposit."); return; }
        if (rawSavingsBalance(userId) - oldAmount + newAmount < -0.000001) {
            puts("This correction would make savings negative because of existing withdrawals."); return;
        }
        users[ui].balance -= increase;
    } else if (transactions[ti].type == SAVINGS_WITHDRAWAL) {
        double savingsWithoutThis = rawSavingsBalance(userId) + oldAmount;
        if (newAmount > savingsWithoutThis + 0.000001) { puts("Corrected withdrawal would exceed the savings available."); return; }
        users[ui].balance += newAmount - oldAmount;
    }
    transactions[ti].amount = newAmount;
    printf("Current date: %s (Enter keeps it)\n", transactions[ti].date);
    dateInput = readLineDynamic("New date (YYYY-MM-DD): ");
    if (dateInput == NULL) { puts("Date unchanged; amount was updated."); }
    else if (dateInput[0] != '\0' && !isValidDate(dateInput)) {
        /* Roll back amount and balance when the date is invalid. */
        if (transactions[ti].type == INCOME) users[ui].balance += oldAmount - newAmount;
        else if (transactions[ti].type == EXPENSE) users[ui].balance += newAmount - oldAmount;
        else if (transactions[ti].type == SAVINGS_DEPOSIT) users[ui].balance += newAmount - oldAmount;
        else if (transactions[ti].type == SAVINGS_WITHDRAWAL) users[ui].balance += oldAmount - newAmount;
        transactions[ti].amount = oldAmount;
        puts("Invalid date; no changes made."); free(dateInput); return;
    } else if (transactions[ti].type == INCOME && users[ui].monthly_budget > 0.0) {
        char today[MAX_DATE_LEN], currentMonth[MAX_MONTH_LEN];
        const char *proposedDate = dateInput[0] != '\0' ? dateInput : transactions[ti].date;
        double proposedMonthlyIncome = 0.0;
        getCurrentDate(today);
        (void)snprintf(currentMonth, sizeof(currentMonth), "%.7s", today);
        for (int k = 0; k < transactionCount; ++k) {
            const Transaction *candidate = &transactions[k];
            const char *candidateDate = (k == ti) ? proposedDate : candidate->date;
            double candidateAmount = (k == ti) ? newAmount : candidate->amount;
            if (candidate->userId == userId && candidate->type == INCOME &&
                strncmp(candidateDate, currentMonth, 7U) == 0) proposedMonthlyIncome += candidateAmount;
        }
        if (proposedMonthlyIncome + 0.000001 < users[ui].monthly_budget) {
            /* Restore the amount/balance update; the user must lower/delete the budget first. */
            users[ui].balance += oldAmount - newAmount;
            transactions[ti].amount = oldAmount;
            puts("This correction would make the monthly budget exceed recorded income. Lower/delete the budget first; no changes made.");
            free(dateInput);
            return;
        }
        if (dateInput[0] != '\0')
            (void)snprintf(transactions[ti].date, sizeof(transactions[ti].date), "%s", dateInput);
    } else if (dateInput[0] != '\0') {
        (void)snprintf(transactions[ti].date, sizeof(transactions[ti].date), "%s", dateInput);
    }
    free(dateInput);
    printf("Current description: %s\n", transactions[ti].description);
    description = readLineDynamic("New description (Enter keeps it): ");
    if (description != NULL && description[0] != '\0') {
        (void)snprintf(transactions[ti].description, sizeof(transactions[ti].description), "%s", description);
        if (transactions[ti].type == INCOME)
            (void)snprintf(transactions[ti].details.incomeSource, sizeof(transactions[ti].details.incomeSource), "%s", description);
        else if (transactions[ti].type == EXPENSE)
            (void)snprintf(transactions[ti].details.expenseNote, sizeof(transactions[ti].details.expenseNote), "%s", description);
    }
    free(description);
    if (transactions[ti].type == EXPENSE) transactions[ti].category = promptCategory();
    refreshUserTotals(userId);
    saveAllData();
    printf("Transaction #%d updated. Available balance: %.2f BDT\n", id, users[ui].balance);
    checkBudgetAlert(userId);
}

void deleteFinancialTransaction(int userId)
{
    int id, ti = -1, ui = findUserById(userId);
    if (ui < 0) { puts("User not found."); return; }
    puts("\n===== DELETE FINANCIAL RECORD =====");
    for (int i = 0; i < transactionCount; ++i) {
        Transaction *t = &transactions[i];
        if (t->userId == userId && t->type != INTERNAL_TRANSACTION)
            printf("ID %d | %-20s | %10.2f BDT | %s\n", t->transactionId, transactionTypeToString(t->type), t->amount, t->description);
    }
    id = readInt("Transaction ID to delete (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    for (int i = 0; i < transactionCount; ++i)
        if (transactions[i].transactionId == id && transactions[i].userId == userId) { ti = i; break; }
    if (ti < 0 || transactions[ti].type == INTERNAL_TRANSACTION) { puts("Record not found or is a linked internal transfer; no changes made."); return; }
    if (isEventLinkedExpense(&transactions[ti])) { puts("This expense belongs to a group event. Delete it through Group Events to preserve settlement calculations."); return; }
    if (transactions[ti].type == INCOME && users[ui].monthly_budget > 0.0) {
        char today[MAX_DATE_LEN];
        getCurrentDate(today);
        if (strncmp(transactions[ti].date, today, 7U) == 0 &&
            calculateTotalIncome(userId) - transactions[ti].amount + 0.000001 < users[ui].monthly_budget) {
            puts("Cannot delete this income because the current budget would exceed the remaining recorded income. Lower/delete the budget first.");
            return;
        }
    }
    printf("Delete #%d (%s, %.2f BDT, %s)?\n", id, transactionTypeToString(transactions[ti].type), transactions[ti].amount, transactions[ti].description);
    if (readInt("Confirm: 1 Yes, 2 No: ", 1, 2) != 1) { puts("Cancelled."); return; }
    if (transactions[ti].type == SAVINGS_DEPOSIT &&
        rawSavingsBalance(userId) - transactions[ti].amount < -0.000001) {
        puts("Cannot delete this deposit because existing withdrawals depend on it. Edit/delete the withdrawal first."); return;
    }
    switch (transactions[ti].type) {
        case INCOME: users[ui].balance -= transactions[ti].amount; break;
        case EXPENSE: users[ui].balance += transactions[ti].amount; break;
        case SAVINGS_DEPOSIT: users[ui].balance += transactions[ti].amount; break;
        case SAVINGS_WITHDRAWAL: users[ui].balance -= transactions[ti].amount; break;
        default: break;
    }
    for (int i = ti; i < transactionCount - 1; ++i) transactions[i] = transactions[i + 1];
    --transactionCount;
    memset(&transactions[transactionCount], 0, sizeof(transactions[transactionCount]));
    refreshUserTotals(userId);
    saveAllData();
    printf("Transaction deleted. Available balance is now %.2f BDT.\n", users[ui].balance);
    checkBudgetAlert(userId);
}
