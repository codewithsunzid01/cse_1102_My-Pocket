#include "budget.h"
#include <limits.h>
#include <math.h>

static int nextRecurringId(void)
{
    int maximum = 0;
    for (int i = 0; i < recurringCount; ++i)
        if (recurringExpenses[i].recurringId > maximum) maximum = recurringExpenses[i].recurringId;
    return maximum + 1;
}

void addRecurringExpense(int userId)
{
    RecurringExpense *r;
    char *title, *date;
    CategoryType category;
    double amount;
    int interval;
    if (!isValidUser(userId)) { puts("User not found."); return; }
    if (recurringCount >= MAX_RECURRING) { puts("Recurring expense storage is full."); return; }
    title = readLineDynamic("Recurring expense title (e.g. rent, internet): ");
    if (title == NULL || title[0] == '\0' || strlen(title) >= MAX_TITLE_LEN) {
        free(title); puts("A non-empty title under 100 characters is required."); return;
    }
    category = promptCategory();
    amount = readDouble("Amount per occurrence (BDT): ", 0.01, 1000000000000.0);
    date = readLineDynamic("Next due date (YYYY-MM-DD): ");
    if (date == NULL || !isValidDate(date)) {
        free(title); free(date); puts("Invalid date. Use YYYY-MM-DD."); return;
    }
    interval = readInt("Repeat interval in days (1-365): ", 1, 365);
    r = &recurringExpenses[recurringCount++];
    memset(r, 0, sizeof(*r));
    r->recurringId = nextRecurringId();
    r->userId = userId;
    r->category = category;
    r->amount = amount;
    r->intervalDays = interval;
    (void)snprintf(r->title, sizeof(r->title), "%s", title);
    (void)snprintf(r->nextDate, sizeof(r->nextDate), "%s", date);
    saveAllData();
    { char displayDate[64]; formatDateDisplay(r->nextDate, displayDate, sizeof(displayDate));
      printf("Recurring expense #%d saved. Next due: %s; repeats every %d day(s).\n", r->recurringId, displayDate, interval); }
    free(title); free(date);
}

void viewRecurringExpenses(int userId)
{
    int found = 0;
    puts("\n================ RECURRING EXPENSES ================");
    printf("%-5s %-12s %-12s %-10s %-7s %s\n", "ID", "Next due", "Amount", "Category", "Every", "Title");
    for (int i = 0; i < recurringCount; ++i) {
        const RecurringExpense *r = &recurringExpenses[i];
        if (r->userId != userId) continue;
        char displayDate[64];
        formatDateDisplay(r->nextDate, displayDate, sizeof(displayDate));
        printf("%-5d %-20s %10.2f %-10s %-7d %s\n", r->recurringId, displayDate,
               r->amount, categoryToString(r->category), r->intervalDays, r->title);
        found = 1;
    }
    if (!found) puts("No recurring expenses have been added.");
}

void processRecurringExpenses(int userId)
{
    char today[MAX_DATE_LEN];
    int processed = 0;
    getCurrentDate(today);
    for (int i = 0; i < recurringCount; ++i) {
        RecurringExpense *r = &recurringExpenses[i];
        if (r->userId != userId || !isValidDate(r->nextDate)) continue;
        while (compareDates(r->nextDate, today) <= 0) {
            int userIndex = findUserById(userId);
            char description[MAX_DESC_LEN];
            if (userIndex < 0 || transactionCount >= MAX_TRANSACTIONS) {
                puts("Cannot process more recurring expenses: user not found or transaction storage is full.");
                saveAllData();
                return;
            }
            (void)snprintf(description, sizeof(description), "Recurring: %.130s", r->title);
            users[userIndex].balance -= r->amount;
            addTransaction(userId, EXPENSE, r->category, r->amount, r->nextDate, description);
            addDaysToDate(r->nextDate, r->intervalDays);
            ++processed;
            /* Defensive guard against invalid interval/date arithmetic. */
            if (!isValidDate(r->nextDate)) break;
        }
    }
    if (processed > 0) {
        saveAllData();
        printf("Processed %d due recurring expense(s).\n", processed);
        checkBudgetAlert(userId);
    }
}

void showUpcomingExpenses(int userId)
{
    int found = 0;
    puts("\n========== UPCOMING EXPENSES (NEXT 30 DAYS) ==========");
    for (int i = 0; i < recurringCount; ++i) {
        const RecurringExpense *r = &recurringExpenses[i];
        int days;
        if (r->userId != userId || !isValidDate(r->nextDate)) continue;
        days = daysUntilDate(r->nextDate);
        if (days >= 0 && days <= 30) {
            printf("%s | %s | %.2f BDT | %s\n", r->nextDate, r->title, r->amount,
                   days == 0 ? "due today" : "scheduled");
            found = 1;
        }
    }
    for (int i = 0; i < reminderCount; ++i) {
        const Reminder *r = &reminders[i];
        int days;
        if (r->userId != userId || r->completed || !isValidDate(r->date)) continue;
        days = daysUntilDate(r->date);
        if (days >= 0 && days <= 30) {
            char displayDate[64];
            formatDateDisplay(r->date, displayDate, sizeof(displayDate));
            printf("Reminder: %s | %s | %s\n", displayDate, r->title, days == 0 ? "today" : "upcoming");
            found = 1;
        }
    }
    if (!found) puts("No recurring expenses or reminders due in the next 30 days.");
}

double calculateUpcomingExpenseTotal(int userId)
{
    double total = 0.0;
    for (int i = 0; i < recurringCount; ++i) {
        const RecurringExpense *r = &recurringExpenses[i];
        int days;
        if (r->userId != userId || !isValidDate(r->nextDate)) continue;
        days = daysUntilDate(r->nextDate);
        if (days >= 0 && days <= 30) total += r->amount;
    }
    return total;
}

void editRecurringExpense(int userId)
{
    int id, index = -1;
    char *title = NULL, *date = NULL, *categoryText = NULL, *amountText = NULL, *intervalText = NULL;
    int newCategory = -1, newInterval = -1;
    double newAmount = -1.0;
    viewRecurringExpenses(userId);
    id = readInt("Recurring expense ID to edit (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    for (int i = 0; i < recurringCount; ++i)
        if (recurringExpenses[i].userId == userId && recurringExpenses[i].recurringId == id) { index = i; break; }
    if (index < 0) { puts("Recurring expense not found in your account."); return; }
    title = readLineDynamic("New title (Enter keeps current): ");
    if (title == NULL) return;
    date = readLineDynamic("New next due date YYYY-MM-DD (Enter keeps current): ");
    if (date == NULL) { free(title); return; }
    categoryText = readLineDynamic("New category 1-9 (Enter keeps current): ");
    if (categoryText == NULL) { free(title); free(date); return; }
    amountText = readLineDynamic("New amount per occurrence (Enter keeps current): ");
    if (amountText == NULL) { free(title); free(date); free(categoryText); return; }
    intervalText = readLineDynamic("New repeat interval in days 1-365 (Enter keeps current): ");
    if (intervalText == NULL) { free(title); free(date); free(categoryText); free(amountText); return; }

    if ((title[0] != '\0' && strlen(title) >= MAX_TITLE_LEN) ||
        (date[0] != '\0' && !isValidDate(date))) {
        puts("Title or date is invalid; no changes made.");
        free(title); free(date); free(categoryText); free(amountText); free(intervalText); return;
    }
    if (categoryText[0] != '\0') {
        char *end = NULL; long category = strtol(categoryText, &end, 10);
        if (end == categoryText || *end != '\0' || category < 1 || category > 9) {
            puts("Category must be 1-9; no changes made.");
            free(title); free(date); free(categoryText); free(amountText); free(intervalText); return;
        }
        newCategory = (int)category - 1;
    }
    if (amountText[0] != '\0') {
        char *end = NULL; double amount = strtod(amountText, &end);
        if (end == amountText || *end != '\0' || !isfinite(amount) || amount <= 0.0 || amount > 1000000000000.0) {
            puts("Amount is invalid; no changes made.");
            free(title); free(date); free(categoryText); free(amountText); free(intervalText); return;
        }
        newAmount = amount;
    }
    if (intervalText[0] != '\0') {
        char *end = NULL; long interval = strtol(intervalText, &end, 10);
        if (end == intervalText || *end != '\0' || interval < 1 || interval > 365) {
            puts("Interval must be 1-365 days; no changes made.");
            free(title); free(date); free(categoryText); free(amountText); free(intervalText); return;
        }
        newInterval = (int)interval;
    }
    /* Apply changes only after every supplied value has passed validation. */
    if (title[0] != '\0') (void)snprintf(recurringExpenses[index].title, sizeof(recurringExpenses[index].title), "%s", title);
    if (date[0] != '\0') (void)snprintf(recurringExpenses[index].nextDate, sizeof(recurringExpenses[index].nextDate), "%s", date);
    if (newCategory >= 0) recurringExpenses[index].category = (CategoryType)newCategory;
    if (newAmount > 0.0) recurringExpenses[index].amount = newAmount;
    if (newInterval > 0) recurringExpenses[index].intervalDays = newInterval;
    saveAllData();
    puts("Recurring expense updated. Past transactions are unchanged; only future scheduled occurrences use these settings.");
    free(title); free(date); free(categoryText); free(amountText); free(intervalText);
}

void deleteRecurringExpense(int userId)
{
    int id, index = -1;
    viewRecurringExpenses(userId);
    id = readInt("Recurring expense ID to delete (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    for (int i = 0; i < recurringCount; ++i)
        if (recurringExpenses[i].userId == userId && recurringExpenses[i].recurringId == id) { index = i; break; }
    if (index < 0) { puts("Recurring expense not found in your account."); return; }
    puts("Deleting this schedule will not delete expenses already processed in transaction history.");
    if (readInt("Delete recurring schedule? 1. Yes  2. No: ", 1, 2) != 1) { puts("Cancelled."); return; }
    for (int i = index; i < recurringCount - 1; ++i) recurringExpenses[i] = recurringExpenses[i + 1];
    --recurringCount;
    memset(&recurringExpenses[recurringCount], 0, sizeof(recurringExpenses[recurringCount]));
    saveAllData();
    puts("Recurring schedule deleted.");
}
