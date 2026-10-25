#include "budget.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

char *readLineDynamic(const char *prompt)
{
    size_t capacity = 64U;
    size_t length = 0U;
    char *line = (char *)malloc(capacity);
    int ch;

    if (line == NULL) {
        fprintf(stderr, "Error: unable to allocate input buffer.\n");
        return NULL;
    }
    if (prompt != NULL) {
        fputs(prompt, stdout);
        fflush(stdout);
    }

    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (length + 1U >= capacity) {
            size_t newCapacity = capacity * 2U;
            char *grown = (char *)realloc(line, newCapacity);
            if (grown == NULL) {
                free(line);
                fprintf(stderr, "Error: unable to grow input buffer.\n");
                return NULL;
            }
            line = grown;
            capacity = newCapacity;
        }
        if (ch != '\r') {
            line[length++] = (char)ch;
        }
    }

    if (ch == EOF && length == 0U) {
        free(line);
        return NULL;
    }
    line[length] = '\0';
    return line;
}

static int onlyWhitespace(const char *text)
{
    while (*text != '\0') {
        if (!isspace((unsigned char)*text)) return 0;
        ++text;
    }
    return 1;
}

int readInt(const char *prompt, int minValue, int maxValue)
{
    for (;;) {
        char *line = readLineDynamic(prompt);
        char *end = NULL;
        long value;
        if (line == NULL) return INT_MIN;
        errno = 0;
        value = strtol(line, &end, 10);
        while (end != NULL && isspace((unsigned char)*end)) ++end;
        if (!onlyWhitespace(line) && errno == 0 && end != line && end != NULL && *end == '\0' &&
            value >= (long)minValue && value <= (long)maxValue) {
            free(line);
            return (int)value;
        }
        free(line);
        printf("Please enter an integer from %d to %d.\n", minValue, maxValue);
    }
}

double readDouble(const char *prompt, double minValue, double maxValue)
{
    for (;;) {
        char *line = readLineDynamic(prompt);
        char *end = NULL;
        double value;
        if (line == NULL) return NAN;
        errno = 0;
        value = strtod(line, &end);
        while (end != NULL && isspace((unsigned char)*end)) ++end;
        if (!onlyWhitespace(line) && errno == 0 && end != line && end != NULL && *end == '\0' &&
            isfinite(value) && value >= minValue && value <= maxValue) {
            free(line);
            return value;
        }
        free(line);
        printf("Please enter an amount from %.2f to %.2f.\n", minValue, maxValue);
    }
}

void getCurrentDate(char *buffer)
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    if (local == NULL) {
        strcpy(buffer, "1970-01-01");
        return;
    }
    (void)strftime(buffer, MAX_DATE_LEN, "%Y-%m-%d", local);
}

static int parseDate(const char *date, struct tm *parsed)
{
    int year, month, day;
    struct tm work;
    time_t converted;
    char trailing;

    if (date == NULL || strlen(date) != 10U ||
        sscanf(date, "%4d-%2d-%2d%c", &year, &month, &day, &trailing) != 3 ||
        date[4] != '-' || date[7] != '-') return 0;
    memset(&work, 0, sizeof(work));
    work.tm_year = year - 1900;
    work.tm_mon = month - 1;
    work.tm_mday = day;
    work.tm_isdst = -1;
    converted = mktime(&work);
    if (converted == (time_t)-1 || work.tm_year != year - 1900 ||
        work.tm_mon != month - 1 || work.tm_mday != day) return 0;
    if (parsed != NULL) *parsed = work;
    return 1;
}

int isValidDate(const char *date)
{
    return parseDate(date, NULL);
}

int compareDates(const char *date1, const char *date2)
{
    int result = strcmp(date1, date2);
    return (result > 0) - (result < 0);
}

int daysUntilDate(const char *date)
{
    struct tm target, today;
    char current[MAX_DATE_LEN];
    time_t targetTime, todayTime;
    double difference;
    if (!parseDate(date, &target)) return INT_MAX;
    getCurrentDate(current);
    if (!parseDate(current, &today)) return INT_MAX;
    target.tm_hour = target.tm_min = target.tm_sec = 0;
    today.tm_hour = today.tm_min = today.tm_sec = 0;
    target.tm_isdst = today.tm_isdst = -1;
    targetTime = mktime(&target);
    todayTime = mktime(&today);
    if (targetTime == (time_t)-1 || todayTime == (time_t)-1) return INT_MAX;
    difference = difftime(targetTime, todayTime) / 86400.0;
    return (int)difference;
}

void addDaysToDate(char *date, int days)
{
    struct tm parsed;
    time_t when;
    if (!parseDate(date, &parsed)) return;
    parsed.tm_mday += days;
    parsed.tm_isdst = -1;
    when = mktime(&parsed);
    if (when != (time_t)-1) {
        (void)strftime(date, MAX_DATE_LEN, "%Y-%m-%d", &parsed);
    }
}

const char *categoryToString(CategoryType category)
{
    static const char *names[] = {"Food", "Transport", "Education", "Medical", "Entertainment", "Shopping", "Rent", "Savings", "Others"};
    if (category < FOOD || category > OTHERS) return "Unknown";
    return names[(int)category];
}

const char *transactionTypeToString(TransactionType type)
{
    switch (type) {
        case INCOME: return "Income";
        case EXPENSE: return "Expense";
        case INTERNAL_TRANSACTION: return "Internal transfer";
        case SAVINGS_DEPOSIT: return "Savings deposit";
        case SAVINGS_WITHDRAWAL: return "Savings withdrawal";
        default: return "Unknown";
    }
}

void formatMonthName(const char *isoMonth, char *buffer, size_t bufferSize)
{
    static const char *names[] = {"January", "February", "March", "April", "May", "June",
                                  "July", "August", "September", "October", "November", "December"};
    int year = 0, month = 0;
    if (buffer == NULL || bufferSize == 0U) return;
    if (isoMonth == NULL || sscanf(isoMonth, "%4d-%2d", &year, &month) != 2 || month < 1 || month > 12) {
        (void)snprintf(buffer, bufferSize, "%s", isoMonth != NULL ? isoMonth : "Unknown month");
        return;
    }
    (void)snprintf(buffer, bufferSize, "%s %04d", names[month - 1], year);
}

void formatDateDisplay(const char *isoDate, char *buffer, size_t bufferSize)
{
    static const char *names[] = {"January", "February", "March", "April", "May", "June",
                                  "July", "August", "September", "October", "November", "December"};
    int year, month, day;
    if (buffer == NULL || bufferSize == 0U) return;
    if (isoDate == NULL || strlen(isoDate) != 10U ||
        sscanf(isoDate, "%4d-%2d-%2d", &year, &month, &day) != 3 || month < 1 || month > 12 || day < 1 || day > 31) {
        (void)snprintf(buffer, bufferSize, "%s", isoDate != NULL ? isoDate : "Unknown date");
        return;
    }
    (void)snprintf(buffer, bufferSize, "%s %d, %04d", names[month - 1], day, year);
}

const char *splitTypeToString(SplitType type)
{
    return type == CUSTOM_SPLIT ? "Custom split" : "Equal split";
}

CategoryType promptCategory(void)
{
    int choice;
    puts("\nExpense category:");
    puts("1. Food  2. Transport  3. Education  4. Medical  5. Entertainment");
    puts("6. Shopping  7. Rent  8. Savings  9. Others");
    choice = readInt("Choose category: ", 1, 9);
    return (CategoryType)(choice - 1);
}

int isValidUser(int userId)
{
    return findUserById(userId) >= 0;
}

void pauseScreen(void)
{
    char *line = readLineDynamic("Press Enter to continue...");
    free(line);
}
