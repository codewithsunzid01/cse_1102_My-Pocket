#ifndef BUDGET_H
#define BUDGET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>

#define MAX_USERS 100
#define MAX_TRANSACTIONS 1000
#define MAX_BUDGETS 100
#define MAX_EVENTS 100
#define MAX_PARTICIPANTS 20
#define MAX_EVENT_EXPENSES 100
#define MAX_FUNDINGS 500
#define MAX_REMINDERS 200
#define MAX_RECURRING 100

#define MAX_NAME_LEN 50
#define MAX_EMAIL_LEN 100
#define MAX_PASSWORD_LEN 50
#define MAX_TITLE_LEN 100
#define MAX_DESC_LEN 150
#define MAX_CATEGORY_LEN 30
#define MAX_DATE_LEN 20
#define MAX_MONTH_LEN 15
#define MAX_TOKENS 20
#define MAX_TOKEN_LEN 30
#define MAX_MESSAGE_LEN 100

typedef enum { INCOME, EXPENSE, INTERNAL_TRANSACTION, SAVINGS_DEPOSIT, SAVINGS_WITHDRAWAL } TransactionType;
typedef enum { FOOD, TRANSPORT, EDUCATION, MEDICAL, ENTERTAINMENT, SHOPPING, RENT, SAVINGS, OTHERS } CategoryType;
typedef enum { EQUAL_SPLIT, CUSTOM_SPLIT } SplitType;
typedef enum { NORMAL_REMINDER, RECURRING_REMINDER } ReminderType;

typedef union {
    char phone[20];
    char studentId[20];
} ExtraInfo;

typedef struct {
    int userId;
    char name[MAX_NAME_LEN];
    char email[MAX_EMAIL_LEN];
    char password[MAX_PASSWORD_LEN];
    double monthly_income;
    double monthly_budget;
    double savings_goal;
    double total_expense;
    double balance;
    ExtraInfo extra;
} User;

typedef struct {
    int budgetId;
    int userId;
    char month[MAX_MONTH_LEN];
    double amount;
    double spent;
    double remaining;
} Budget;

/* Transaction-specific notes demonstrate a union in the transaction model.
   Only the member matching Transaction.type is active at a time. */
typedef union {
    char incomeSource[MAX_DESC_LEN];
    char expenseNote[MAX_DESC_LEN];
    char internalNote[MAX_DESC_LEN];
} TransactionDetails;

typedef struct {
    int transactionId;
    int userId;
    TransactionType type;
    CategoryType category;
    double amount;
    char date[MAX_DATE_LEN];
    char description[MAX_DESC_LEN];
    TransactionDetails details;
} Transaction;

typedef struct {
    int reminderId;
    int userId;
    char title[MAX_TITLE_LEN];
    char date[MAX_DATE_LEN];
    ReminderType type;
    int completed;
} Reminder;

typedef struct {
    int recurringId;
    int userId;
    char title[MAX_TITLE_LEN];
    CategoryType category;
    double amount;
    char nextDate[MAX_DATE_LEN];
    int intervalDays;
} RecurringExpense;

typedef struct {
    int userId;
    char name[MAX_NAME_LEN];
} EventParticipant;

typedef struct {
    int userId;
    double shareAmount;
} ExpenseShare;

typedef struct {
    int expenseId;
    int eventId;
    char title[MAX_TITLE_LEN];
    double totalAmount;
    int paidBy;
    SplitType splitType;
    int participantCount;
    ExpenseShare shares[MAX_PARTICIPANTS];
    char date[MAX_DATE_LEN];
} EventExpense;

typedef struct {
    int eventId;
    char eventName[MAX_TITLE_LEN];
    char eventDate[MAX_DATE_LEN];
    int createdBy;
    int participantCount;
    EventParticipant participants[MAX_PARTICIPANTS];
    int expenseCount;
    EventExpense expenses[MAX_EVENT_EXPENSES];
} Event;

typedef struct {
    int fundingId;
    int eventId;
    int fromUserId;
    int toUserId;
    double amount;
    char date[MAX_DATE_LEN];
    TransactionType type;
    char description[MAX_DESC_LEN];
} Funding;

typedef struct {
    int userId;
    char month[MAX_MONTH_LEN];
    double totalIncome;
    double totalExpense;
    double totalSavings;
    double currentBalance;
    double futureBalance;
} FinancialSummary;

typedef struct {
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int count;
} TokenList;

extern User users[MAX_USERS];
extern int userCount;
extern Budget budgets[MAX_BUDGETS];
extern int budgetCount;
extern Transaction transactions[MAX_TRANSACTIONS];
extern int transactionCount;
extern Event events[MAX_EVENTS];
extern int eventCount;
extern Funding fundings[MAX_FUNDINGS];
extern int fundingCount;
extern Reminder reminders[MAX_REMINDERS];
extern int reminderCount;
extern RecurringExpense recurringExpenses[MAX_RECURRING];
extern int recurringCount;

/* Main/menu and input helpers */
void showMainMenu(void);
void showDashboard(int userId);
void user_menu(int userId);
void pauseScreen(void);
char *readLineDynamic(const char *prompt);
int readInt(const char *prompt, int minValue, int maxValue);
double readDouble(const char *prompt, double minValue, double maxValue);
void getCurrentDate(char *buffer);
int isValidDate(const char *date);
int compareDates(const char *date1, const char *date2);
int daysUntilDate(const char *date);
void addDaysToDate(char *date, int days);
const char *categoryToString(CategoryType category);
const char *transactionTypeToString(TransactionType type);
const char *splitTypeToString(SplitType type);
void formatMonthName(const char *isoMonth, char *buffer, size_t bufferSize);
void formatDateDisplay(const char *isoDate, char *buffer, size_t bufferSize);
CategoryType promptCategory(void);
int isValidUser(int userId);

/* Authentication */
int registerUser(void);
void editProfile(int userId);
int loginUser(void);
int authenticateUser(const char *email, const char *password);
int findUserById(int userId);
int findUserByEmail(const char *email);
int isStrongPassword(const char *password);

/* File handling */
void loadAllData(void);
void saveAllData(void);
void loadUsers(void);
void saveUsers(void);
void loadBudgets(void);
void saveBudgets(void);
void loadTransactions(void);
void saveTransactions(void);
void loadEvents(void);
void saveEvents(void);
void loadFundings(void);
void saveFundings(void);
void loadReminders(void);
void saveReminders(void);
void loadRecurringExpenses(void);
void saveRecurringExpenses(void);

/* Budget and income */
void manageIncome(int userId);
void manageBudget(int userId);
void deleteCurrentBudget(int userId);
void addIncome(int userId, double amount);
void setBudget(int userId, double amount);
double calculateRemainingBudget(int userId);

/* Expenses and transactions */
void addExpense(int userId);
void editExpense(int userId);
void editFinancialTransaction(int userId);
void deleteFinancialTransaction(int userId);
void addTransaction(int userId, TransactionType type, CategoryType category, double amount, const char *date, const char *description);
void viewTransactions(int userId);
void viewTransactionsByCategory(int userId, CategoryType category);

/* Financial summary */
void showFinancialSummary(int userId);
double calculateTotalIncome(int userId);
double calculateTotalExpense(int userId);
double calculateCurrentBalance(int userId);
double calculateFutureBalance(int userId);
void showSectorWiseExpenses(int userId);
double getCategoryExpenseTotal(int userId, CategoryType category);
void showMonthlyReview(int userId);
void exportTransactionsCsv(int userId);

/* Search and tokenization */
void searchTransactions(int userId);
TokenList tokenizeString(const char *input, const char *delimiter);
int stringContainsToken(const char *text, const char *token);
void search_and_tokenization_demo(void);

/* Calendar and reminders */
void addReminder(int userId);
void viewReminders(int userId);
void markReminderCompleted(int userId);
void editReminder(int userId);
void deleteReminder(int userId);
void checkUpcomingReminders(int userId);
void reminder_dashboard(int userId);

/* Recurring and upcoming expenses */
void addRecurringExpense(int userId);
void viewRecurringExpenses(int userId);
void editRecurringExpense(int userId);
void deleteRecurringExpense(int userId);
void processRecurringExpenses(int userId);
void showUpcomingExpenses(int userId);
double calculateUpcomingExpenseTotal(int userId);

/* Group event and expense sharing */
void createEvent(int userId);
void editEvent(int userId);
void deleteEvent(int userId);
void viewEvents(int userId);
int findEventById(int eventId);
void addParticipantToEvent(int eventId);
void removeParticipantFromEvent(int eventId);
void addEventExpense(int eventId);
void editEventExpense(int eventId);
void deleteEventExpense(int eventId);
void splitExpenseEqual(Event *event, EventExpense *expense);
void splitExpenseCustom(Event *event, EventExpense *expense);
void viewEventDetails(int eventId);
void event_dashboard(int userId);
double calculateEventNet(int eventId, int userId);

/* Internal funding and transfers */
void collectFund(int eventId, int collectorUserId);
void editFunding(int eventId);
void deleteFunding(int eventId);
void deductFromUser(int userId, double amount);
void addToUser(int userId, double amount);
void recordInternalTransaction(int fromUserId, int toUserId, double amount, const char *date, const char *description);

/* Alerts and planning */
void checkBudgetAlert(int userId);
void showFinancialPlanning(int userId);
void savingsManagement(int userId);
double calculateSavingsBalance(int userId);
void showEventSettlementSuggestions(int eventId);

#endif
