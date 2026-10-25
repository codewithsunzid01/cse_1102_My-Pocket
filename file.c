#include "budget.h"
#include <errno.h>
#include <ctype.h>
#ifdef _WIN32
#include <direct.h>
#define MAKE_DATA_DIR() _mkdir("data")
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MAKE_DATA_DIR() mkdir("data", 0755)
#endif

/*
 * Text persistence format:
 *   MYPocket-TEXT-HEX-v1 <record-size> <record-count>
 *   <one hex-encoded record per line>
 *
 * This keeps every struct (including nested event arrays and unions) intact
 * while storing it in ordinary .txt files. The hex lines are text, but are
 * intentionally machine-oriented rather than human-readable field lists.
 */
static void ensureDataDirectory(void)
{
    (void)MAKE_DATA_DIR(); /* already-existing directory is fine */
}

static int saveArrayText(const char *path, const void *array, size_t count, size_t itemSize)
{
    const unsigned char *bytes = (const unsigned char *)array;
    FILE *file = fopen(path, "w");
    size_t i, j;
    if (file == NULL) {
        fprintf(stderr, "Could not save %s. Check write permission.\n", path);
        return 0;
    }
    if (fprintf(file, "MYPocket-TEXT-HEX-v1 %zu %zu\n", itemSize, count) < 0) {
        fclose(file);
        return 0;
    }
    for (i = 0; i < count; ++i) {
        const unsigned char *record = bytes + i * itemSize;
        for (j = 0; j < itemSize; ++j) {
            if (fprintf(file, "%02X", (unsigned int)record[j]) < 0) {
                fclose(file);
                fprintf(stderr, "Warning: incomplete write to %s.\n", path);
                return 0;
            }
        }
        if (fputc('\n', file) == EOF) {
            fclose(file);
            return 0;
        }
    }
    if (fclose(file) != 0) {
        fprintf(stderr, "Warning: could not close %s cleanly.\n", path);
        return 0;
    }
    return 1;
}

static int hexValue(int ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    ch = toupper((unsigned char)ch);
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

static void loadArrayText(const char *path, void *array, size_t capacity,
                          size_t itemSize, int *countOut)
{
    FILE *file = fopen(path, "r");
    char header[128];
    size_t storedSize = 0U, count = 0U, i, j;
    unsigned char *bytes = (unsigned char *)array;
    *countOut = 0;
    if (file == NULL) return; /* first run */
    if (fgets(header, sizeof(header), file) == NULL ||
        sscanf(header, "MYPocket-TEXT-HEX-v1 %zu %zu", &storedSize, &count) != 2 ||
        storedSize != itemSize || count > 1000000U) {
        fprintf(stderr, "Warning: invalid or incompatible text data file %s; starting empty.\n", path);
        fclose(file);
        return;
    }
    if (count > capacity) {
        fprintf(stderr, "Warning: %s has more records than supported; extra records ignored.\n", path);
        count = capacity;
    }
    for (i = 0; i < count; ++i) {
        int hi, lo;
        for (j = 0; j < itemSize; ++j) {
            hi = fgetc(file);
            if (hi == EOF || (hi = hexValue(hi)) < 0) goto malformed;
            lo = fgetc(file);
            if (lo == EOF || (lo = hexValue(lo)) < 0) goto malformed;
            bytes[i * itemSize + j] = (unsigned char)((hi << 4) | lo);
        }
        /* Every encoded record must end with a newline. */
        if (fgetc(file) != '\n') goto malformed;
        *countOut = (int)(i + 1U);
    }
    fclose(file);
    return;

malformed:
    fprintf(stderr, "Warning: malformed record in %s; loaded %d complete record(s).\n", path, *countOut);
    fclose(file);
}

void loadUsers(void) { loadArrayText("data/users.txt", users, MAX_USERS, sizeof(User), &userCount); }
void saveUsers(void) { ensureDataDirectory(); (void)saveArrayText("data/users.txt", users, (size_t)userCount, sizeof(User)); }
void loadBudgets(void) { loadArrayText("data/budgets.txt", budgets, MAX_BUDGETS, sizeof(Budget), &budgetCount); }
void saveBudgets(void) { ensureDataDirectory(); (void)saveArrayText("data/budgets.txt", budgets, (size_t)budgetCount, sizeof(Budget)); }
void loadTransactions(void) { loadArrayText("data/transactions.txt", transactions, MAX_TRANSACTIONS, sizeof(Transaction), &transactionCount); }
void saveTransactions(void) { ensureDataDirectory(); (void)saveArrayText("data/transactions.txt", transactions, (size_t)transactionCount, sizeof(Transaction)); }
void loadEvents(void) { loadArrayText("data/events.txt", events, MAX_EVENTS, sizeof(Event), &eventCount); }
void saveEvents(void) { ensureDataDirectory(); (void)saveArrayText("data/events.txt", events, (size_t)eventCount, sizeof(Event)); }
void loadFundings(void) { loadArrayText("data/fundings.txt", fundings, MAX_FUNDINGS, sizeof(Funding), &fundingCount); }
void saveFundings(void) { ensureDataDirectory(); (void)saveArrayText("data/fundings.txt", fundings, (size_t)fundingCount, sizeof(Funding)); }
void loadReminders(void) { loadArrayText("data/reminders.txt", reminders, MAX_REMINDERS, sizeof(Reminder), &reminderCount); }
void saveReminders(void) { ensureDataDirectory(); (void)saveArrayText("data/reminders.txt", reminders, (size_t)reminderCount, sizeof(Reminder)); }
void loadRecurringExpenses(void) { loadArrayText("data/recurring.txt", recurringExpenses, MAX_RECURRING, sizeof(RecurringExpense), &recurringCount); }
void saveRecurringExpenses(void) { ensureDataDirectory(); (void)saveArrayText("data/recurring.txt", recurringExpenses, (size_t)recurringCount, sizeof(RecurringExpense)); }

void loadAllData(void)
{
    ensureDataDirectory();
    loadUsers();
    loadBudgets();
    loadTransactions();
    loadEvents();
    loadFundings();
    loadReminders();
    loadRecurringExpenses();
}

void saveAllData(void)
{
    ensureDataDirectory();
    saveUsers();
    saveBudgets();
    saveTransactions();
    saveEvents();
    saveFundings();
    saveReminders();
    saveRecurringExpenses();
}
