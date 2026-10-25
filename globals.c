#include "budget.h"

User users[MAX_USERS];
int userCount = 0;

Budget budgets[MAX_BUDGETS];
int budgetCount = 0;

Transaction transactions[MAX_TRANSACTIONS];
int transactionCount = 0;

Event events[MAX_EVENTS];
int eventCount = 0;

Funding fundings[MAX_FUNDINGS];
int fundingCount = 0;

Reminder reminders[MAX_REMINDERS];
int reminderCount = 0;

RecurringExpense recurringExpenses[MAX_RECURRING];
int recurringCount = 0;