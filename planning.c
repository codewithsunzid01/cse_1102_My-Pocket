#include "budget.h"
#include <math.h>

void checkBudgetAlert(int userId)
{
    int index = findUserById(userId);
    double expense, ratio;
    if (index < 0) { puts("User not found."); return; }
    if (users[index].monthly_budget <= 0.0) {
        puts("Budget alert: you have not set a monthly budget yet.");
        return;
    }
    expense = calculateTotalExpense(userId);
    ratio = expense / users[index].monthly_budget;
    if (ratio >= 1.0) {
        printf("BUDGET ALERT: you have exceeded your budget by %.2f BDT.\n", expense - users[index].monthly_budget);
    } else if (ratio >= 0.8) {
        printf("Budget warning: %.1f%% used; only %.2f BDT remains.\n", ratio * 100.0, users[index].monthly_budget - expense);
    } else {
        printf("Budget status: %.1f%% used; %.2f BDT remains.\n", ratio * 100.0, users[index].monthly_budget - expense);
    }
}

double calculateFutureBalance(int userId)
{
    int index = findUserById(userId);
    if (index < 0) return 0.0;
    return users[index].balance - calculateUpcomingExpenseTotal(userId);
}

void showFinancialPlanning(int userId)
{
    int index = findUserById(userId);
    int choice;
    if (index < 0) { puts("User not found."); return; }
    for (;;) {
        double income = calculateTotalIncome(userId);
        double expense = calculateTotalExpense(userId);
        puts("\n=============== FINANCIAL PLANNING ===============");
        printf("Monthly net amount: %.2f BDT\n", income - expense);
        printf("Available balance: %.2f BDT\n", users[index].balance);
        printf("Upcoming recurring expenses: %.2f BDT\n", calculateUpcomingExpenseTotal(userId));
        printf("Projected balance after listed upcoming expenses: %.2f BDT\n", calculateFutureBalance(userId));
        printf("Actual savings balance: %.2f BDT\n", calculateSavingsBalance(userId));
        printf("Savings goal: %.2f BDT\n", users[index].savings_goal);
        if (users[index].savings_goal > 0.0) {
            double gap = users[index].savings_goal - calculateSavingsBalance(userId);
            if (gap <= 0.0) puts("Savings goal reached!");
            else printf("Still needed to reach the goal: %.2f BDT\n", gap);
        }
        puts("1. Set/update/clear savings goal (0 clears)\n2. View upcoming expenses\n3. Check budget alert\n4. Back");
        choice = readInt("Choose: ", 1, 4);
        if (choice == 1) {
            users[index].savings_goal = readDouble("Enter target savings balance (BDT): ", 0.0, 1000000000000.0);
            saveAllData();
            puts("Savings goal saved.");
        } else if (choice == 2) showUpcomingExpenses(userId);
        else if (choice == 3) checkBudgetAlert(userId);
        else return;
    }
}


double calculateSavingsBalance(int userId)
{
    double balance = 0.0;
    for (int i = 0; i < transactionCount; ++i) {
        if (transactions[i].userId != userId) continue;
        if (transactions[i].type == SAVINGS_DEPOSIT) balance += transactions[i].amount;
        else if (transactions[i].type == SAVINGS_WITHDRAWAL) balance -= transactions[i].amount;
    }
    return balance > 0.0 ? balance : 0.0;
}

void savingsManagement(int userId)
{
    int index = findUserById(userId), choice;
    if (index < 0) { puts("User not found."); return; }
    for (;;) {
        double saved = calculateSavingsBalance(userId);
        double goal = users[index].savings_goal;
        puts("\n=============== SAVINGS MANAGEMENT ===============");
        printf("Current savings balance : %.2f BDT\n", saved);
        printf("Savings goal             : %.2f BDT\n", goal);
        if (goal > 0.0) {
            double progress = saved * 100.0 / goal;
            if (progress > 100.0) progress = 100.0;
            printf("Goal progress            : %.1f%%\n", progress);
            printf("Amount still needed      : %.2f BDT\n", saved >= goal ? 0.0 : goal - saved);
        } else {
            puts("No savings goal set yet.");
        }
        puts("1. Add money to savings\n2. Withdraw from savings\n3. Set/update/clear savings goal (0 clears)\n4. View savings transactions\n5. Edit a financial record\n6. Delete a financial record\n7. Back");
        choice = readInt("Choose: ", 1, 7);
        if (choice == 1) {
            double amount = readDouble("Amount to move into savings (BDT): ", 0.01, 1000000000000.0);
            if (!isfinite(amount)) continue;
            if (transactionCount >= MAX_TRANSACTIONS) { puts("Transaction storage is full."); continue; }
            if (users[index].balance + 0.000001 < amount) {
                puts("Insufficient available balance. Add income or choose a smaller amount.");
                continue;
            }
            users[index].balance -= amount;
            addTransaction(userId, SAVINGS_DEPOSIT, SAVINGS, amount, NULL, "Moved money into savings");
            saveAllData();
            printf("Saved %.2f BDT. Available balance: %.2f BDT\n", amount, users[index].balance);
        } else if (choice == 2) {
            double amount = readDouble("Amount to withdraw from savings (BDT): ", 0.01, 1000000000000.0);
            if (!isfinite(amount)) continue;
            saved = calculateSavingsBalance(userId);
            if (amount > saved + 0.000001) {
                printf("Insufficient savings. Current savings: %.2f BDT\n", saved);
                continue;
            }
            if (transactionCount >= MAX_TRANSACTIONS) { puts("Transaction storage is full."); continue; }
            users[index].balance += amount;
            addTransaction(userId, SAVINGS_WITHDRAWAL, SAVINGS, amount, NULL, "Withdrawn from savings");
            saveAllData();
            printf("Withdrew %.2f BDT. Available balance: %.2f BDT\n", amount, users[index].balance);
        } else if (choice == 3) {
            double target = readDouble("Enter savings target (BDT, 0 to clear goal): ", 0.0, 1000000000000.0);
            if (!isfinite(target)) continue;
            users[index].savings_goal = target;
            saveAllData();
            puts("Savings goal updated.");
        } else if (choice == 4) {
            int found = 0;
            puts("\nSavings transactions:");
            for (int i = 0; i < transactionCount; ++i) {
                const Transaction *t = &transactions[i];
                if (t->userId == userId && (t->type == SAVINGS_DEPOSIT || t->type == SAVINGS_WITHDRAWAL)) {
                    printf("#%d | %s | %s | %.2f BDT | %s\n", t->transactionId, t->date,
                           transactionTypeToString(t->type), t->amount, t->description);
                    found = 1;
                }
            }
            if (!found) puts("No savings transactions yet.");
        } else if (choice == 5) {
            editFinancialTransaction(userId);
        } else if (choice == 6) {
            deleteFinancialTransaction(userId);
        } else {
            return;
        }
    }
}
