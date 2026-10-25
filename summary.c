#include "budget.h"
#include <math.h>

static double sumCategoryForMonth(int userId, CategoryType category, const char *month);

/* Recursive traversal demonstrates recursion while keeping calculations consistent. */
static double sumTransactionsForMonth(int userId, TransactionType type, CategoryType category,
                                      int categoryFilter, const char *month, int index)
{
    double current = 0.0;
    if (index < 0) return 0.0;
    if (transactions[index].userId == userId && transactions[index].type == type &&
        strncmp(transactions[index].date, month, 7U) == 0 &&
        (!categoryFilter || transactions[index].category == category)) {
        current = transactions[index].amount;
    }
    return current + sumTransactionsForMonth(userId, type, category, categoryFilter, month, index - 1);
}

static void currentMonthString(char *month)
{
    char date[MAX_DATE_LEN];
    getCurrentDate(date);
    (void)snprintf(month, MAX_MONTH_LEN, "%.7s", date);
}

static void previousMonthString(const char *month, char *previous)
{
    int year = 0, number = 0;
    (void)sscanf(month, "%4d-%2d", &year, &number);
    if (number <= 1) { --year; number = 12; } else --number;
    if (year < 1 || year > 9999) year = 2000;
    if (number < 1 || number > 12) number = 1;
    previous[0] = (char)('0' + (year / 1000) % 10);
    previous[1] = (char)('0' + (year / 100) % 10);
    previous[2] = (char)('0' + (year / 10) % 10);
    previous[3] = (char)('0' + year % 10);
    previous[4] = '-';
    previous[5] = (char)('0' + number / 10);
    previous[6] = (char)('0' + number % 10);
    previous[7] = '\0';
}

static double sumMonthFlat(int userId, TransactionType type, const char *month)
{
    double total = 0.0;
    for (int i = 0; i < transactionCount; ++i) {
        if (transactions[i].userId == userId && transactions[i].type == type &&
            strncmp(transactions[i].date, month, 7U) == 0) total += transactions[i].amount;
    }
    return total;
}

double calculateTotalIncome(int userId)
{
    char month[MAX_MONTH_LEN];
    currentMonthString(month);
    return sumTransactionsForMonth(userId, INCOME, OTHERS, 0, month, transactionCount - 1);
}

double calculateTotalExpense(int userId)
{
    char month[MAX_MONTH_LEN];
    currentMonthString(month);
    return sumTransactionsForMonth(userId, EXPENSE, OTHERS, 0, month, transactionCount - 1);
}

double calculateCurrentBalance(int userId)
{
    int index = findUserById(userId);
    return index >= 0 ? users[index].balance : 0.0;
}

double getCategoryExpenseTotal(int userId, CategoryType category)
{
    char month[MAX_MONTH_LEN];
    currentMonthString(month);
    return sumTransactionsForMonth(userId, EXPENSE, category, 1, month, transactionCount - 1);
}

void showSectorWiseExpenses(int userId)
{
    double all = calculateTotalExpense(userId);
    puts("\n========== THIS MONTH'S SECTOR-WISE EXPENSES ==========");
    for (int category = FOOD; category <= OTHERS; ++category) {
        double amount = getCategoryExpenseTotal(userId, (CategoryType)category);
        if (amount > 0.0) {
            printf("%-16s %10.2f BDT", categoryToString((CategoryType)category), amount);
            if (all > 0.0) printf(" (%5.1f%%)", amount * 100.0 / all);
            putchar('\n');
        }
    }
    printf("Total monthly expenses: %.2f BDT\n", all);
}

void showFinancialSummary(int userId)
{
    int index = findUserById(userId);
    char month[MAX_MONTH_LEN];
    unsigned int flags = 0U;
    const unsigned int HAS_INCOME = 1U << 0;
    const unsigned int HAS_BUDGET = 1U << 1;
    const unsigned int HAS_EXPENSE = 1U << 2;
    double income, expense, budgetRemaining, balance, upcoming;
    if (index < 0) { puts("User not found."); return; }
    currentMonthString(month);
    income = calculateTotalIncome(userId);
    expense = calculateTotalExpense(userId);
    budgetRemaining = calculateRemainingBudget(userId);
    balance = calculateCurrentBalance(userId);
    upcoming = calculateUpcomingExpenseTotal(userId);
    if (income > 0.0) flags |= HAS_INCOME;
    if (users[index].monthly_budget > 0.0) flags |= HAS_BUDGET;
    if (expense > 0.0) flags |= HAS_EXPENSE;

    puts("\n================ FINANCIAL SUMMARY ================");
    { char displayMonth[40]; formatMonthName(month, displayMonth, sizeof(displayMonth));
      printf("Profile: %s (ID %d)\nMonth: %s\n", users[index].name, userId, displayMonth); }
    printf("Income recorded this month : %10.2f BDT\n", income);
    printf("Expenses this month        : %10.2f BDT\n", expense);
    printf("Net this month             : %10.2f BDT\n", income - expense);
    printf("Current account balance    : %10.2f BDT\n", balance);
    if ((flags & HAS_BUDGET) != 0U) {
        printf("Monthly budget             : %10.2f BDT\n", users[index].monthly_budget);
        printf("Budget remaining           : %10.2f BDT\n", budgetRemaining);
    } else {
        puts("Monthly budget             : Not set");
    }
    printf("Upcoming recurring costs   : %10.2f BDT\n", upcoming);
    printf("Projected balance          : %10.2f BDT\n", calculateFutureBalance(userId));
    printf("Status flags (bitwise lab) : income=%s, budget=%s, expenses=%s\n",
           (flags & HAS_INCOME) != 0U ? "yes" : "no",
           (flags & HAS_BUDGET) != 0U ? "yes" : "no",
           (flags & HAS_EXPENSE) != 0U ? "yes" : "no");
    if (expense > income) puts("Note: your expenses are higher than this month's recorded income.");
    saveAllData();
}

void showMonthlyReview(int userId)
{
    int index = findUserById(userId);
    char month[MAX_MONTH_LEN], previous[MAX_MONTH_LEN];
    double income, expense, previousIncome, previousExpense, savings;
    CategoryType highest = FOOD;
    double highestAmount = 0.0;
    if (index < 0) { puts("User not found."); return; }
    currentMonthString(month);
    previousMonthString(month, previous);
    income = calculateTotalIncome(userId);
    expense = calculateTotalExpense(userId);
    previousIncome = sumMonthFlat(userId, INCOME, previous);
    previousExpense = sumMonthFlat(userId, EXPENSE, previous);
    savings = income - expense;
    for (int category = FOOD; category <= OTHERS; ++category) {
        double value = getCategoryExpenseTotal(userId, (CategoryType)category);
        if (value > highestAmount) { highestAmount = value; highest = (CategoryType)category; }
    }
    puts("\n================ MONTHLY REVIEW ================");
    { char displayMonth[40], displayPrevious[40];
      formatMonthName(month, displayMonth, sizeof(displayMonth));
      formatMonthName(previous, displayPrevious, sizeof(displayPrevious));
      printf("Review month: %s\n", displayMonth);
    printf("Income: %.2f BDT | Expenses: %.2f BDT | Net saving: %.2f BDT\n", income, expense, savings);
    if (income > 0.0) printf("Savings rate: %.1f%% of income\n", savings * 100.0 / income);
    else puts("Savings rate: unavailable (no income recorded this month)");
    if (highestAmount > 0.0) printf("Highest expense sector: %s (%.2f BDT)\n", categoryToString(highest), highestAmount);
    else puts("No expenses recorded this month.");
      printf("Previous month (%s): income %.2f BDT, expenses %.2f BDT\n", displayPrevious, previousIncome, previousExpense);
      puts("Category comparison (this month vs previous month):");
      for (int category = FOOD; category <= OTHERS; ++category) {
          double nowAmount = sumCategoryForMonth(userId, (CategoryType)category, month);
          double prevAmount = sumCategoryForMonth(userId, (CategoryType)category, previous);
          if (nowAmount > 0.0 || prevAmount > 0.0)
              printf("  %-16s %8.2f vs %8.2f BDT\n", categoryToString((CategoryType)category), nowAmount, prevAmount);
      }
    }
    if (expense > previousExpense) printf("Expenses increased by %.2f BDT compared with last month.\n", expense - previousExpense);
    else printf("Expenses decreased by %.2f BDT (or stayed lower) compared with last month.\n", previousExpense - expense);
    if (users[index].monthly_budget > 0.0) {
        double remaining = calculateRemainingBudget(userId);
        printf("Budget utilisation: %.1f%%\n", expense * 100.0 / users[index].monthly_budget);
        printf("Remaining budget: %.2f BDT\n", remaining);
    }
}


static double sumCategoryForMonth(int userId, CategoryType category, const char *month)
{
    double total = 0.0;
    for (int i = 0; i < transactionCount; ++i) {
        const Transaction *t = &transactions[i];
        if (t->userId == userId && t->type == EXPENSE && t->category == category &&
            strncmp(t->date, month, 7U) == 0) total += t->amount;
    }
    return total;
}

static void writeCsvField(FILE *file, const char *value)
{
    fputc('"', file);
    if (value != NULL) {
        while (*value != '\0') {
            if (*value == '"') fputc('"', file);
            fputc(*value++, file);
        }
    }
    fputc('"', file);
}

void exportTransactionsCsv(int userId)
{
    char path[80];
    FILE *file;
    int exported = 0;
    if (!isValidUser(userId)) { puts("User not found."); return; }
    (void)snprintf(path, sizeof(path), "data/transactions_user_%d.csv", userId);
    file = fopen(path, "w");
    if (file == NULL) { printf("Could not create %s. Check the data folder permissions.\n", path); return; }
    fputs("Transaction ID,Date,Type,Category,Amount,Description\n", file);
    for (int i = 0; i < transactionCount; ++i) {
        const Transaction *t = &transactions[i];
        if (t->userId != userId) continue;
        fprintf(file, "%d,", t->transactionId);
        writeCsvField(file, t->date); fputc(',', file);
        writeCsvField(file, transactionTypeToString(t->type)); fputc(',', file);
        writeCsvField(file, categoryToString(t->category));
        fprintf(file, ",%.2f,", t->amount);
        writeCsvField(file, t->description);
        fputc('\n', file);
        ++exported;
    }
    if (fclose(file) != 0) { puts("CSV export could not be completed cleanly."); return; }
    printf("Exported %d transaction(s) to %s. Open this file with Excel or another spreadsheet app.\n", exported, path);
}
