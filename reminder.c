#include "budget.h"
#include <limits.h>

static int nextReminderId(void)
{
    int maximum = 0;
    for (int i = 0; i < reminderCount; ++i)
        if (reminders[i].reminderId > maximum) maximum = reminders[i].reminderId;
    return maximum + 1;
}

void addReminder(int userId)
{
    Reminder *reminder;
    char *title, *date;
    int type;

    if (!isValidUser(userId)) { puts("User not found."); return; }
    if (reminderCount >= MAX_REMINDERS) { puts("Reminder storage is full."); return; }

    title = readLineDynamic("Reminder title: ");
    if (title == NULL || title[0] == '\0' || strlen(title) >= MAX_TITLE_LEN) {
        free(title); puts("A non-empty title under 100 characters is required."); return;
    }
    date = readLineDynamic("Date (YYYY-MM-DD): ");
    if (date == NULL || !isValidDate(date) || strlen(date) >= MAX_DATE_LEN) {
        free(title); free(date); puts("Invalid date. Use YYYY-MM-DD."); return;
    }
    type = readInt("Type: 1. Normal reminder  2. Recurring reminder label: ", 1, 2);

    reminder = &reminders[reminderCount++];
    memset(reminder, 0, sizeof(*reminder));
    reminder->reminderId = nextReminderId();
    reminder->userId = userId;
    (void)snprintf(reminder->title, sizeof(reminder->title), "%s", title);
    (void)snprintf(reminder->date, sizeof(reminder->date), "%s", date);
    reminder->type = type == 1 ? NORMAL_REMINDER : RECURRING_REMINDER;
    reminder->completed = 0;
    saveAllData();
    { char displayDate[64]; formatDateDisplay(reminder->date, displayDate, sizeof(displayDate));
      printf("Reminder #%d saved for %s.\n", reminder->reminderId, displayDate); }
    if (reminder->type == RECURRING_REMINDER)
        puts("Note: this label does not auto-repeat; use Recurring Expenses for automatic scheduled expenses.");
    free(title);
    free(date);
}

void viewReminders(int userId)
{
    int found = 0;
    puts("\n================ YOUR REMINDERS ================");
    printf("%-5s %-12s %-10s %-9s %s\n", "ID", "Date", "Status", "Type", "Title");
    for (int i = 0; i < reminderCount; ++i) {
        const Reminder *r = &reminders[i];
        if (r->userId != userId) continue;
        printf("%-5d %-12s %-10s %-9s %s\n", r->reminderId, r->date,
               r->completed ? "Completed" : "Pending",
               r->type == RECURRING_REMINDER ? "Recurring" : "Normal", r->title);
        found = 1;
    }
    if (!found) puts("No reminders created yet.");
}

void markReminderCompleted(int userId)
{
    int id = readInt("Enter reminder ID to mark complete: ", 1, INT_MAX);
    for (int i = 0; i < reminderCount; ++i) {
        if (reminders[i].userId == userId && reminders[i].reminderId == id) {
            if (reminders[i].completed) puts("This reminder is already completed.");
            else {
                reminders[i].completed = 1;
                saveAllData();
                puts("Reminder marked as completed.");
            }
            return;
        }
    }
    puts("Reminder not found in your account.");
}

void checkUpcomingReminders(int userId)
{
    int found = 0;
    puts("\n========== UPCOMING / OVERDUE REMINDERS ==========");
    for (int i = 0; i < reminderCount; ++i) {
        const Reminder *r = &reminders[i];
        int days;
        if (r->userId != userId || r->completed) continue;
        days = daysUntilDate(r->date);
        if (days <= 7) {
            if (days < 0) printf("OVERDUE by %d day(s): ", -days);
            else if (days == 0) printf("DUE TODAY: ");
            else printf("Due in %d day(s): ", days);
            char displayDate[64];
            formatDateDisplay(r->date, displayDate, sizeof(displayDate));
            printf("%s (%s)\n", r->title, displayDate);
            found = 1;
        }
    }
    if (!found) puts("No overdue or upcoming reminders in the next 7 days.");
}

void reminder_dashboard(int userId)
{
    int choice;
    for (;;) {
        puts("\n===== REMINDER & CALENDAR DASHBOARD =====");
        puts("1. Add reminder\n2. View my reminders\n3. Mark reminder completed\n4. Check upcoming reminders\n5. Edit reminder\n6. Delete reminder\n7. Back");
        choice = readInt("Choose: ", 1, 7);
        switch (choice) {
            case 1: addReminder(userId); break;
            case 2: viewReminders(userId); break;
            case 3: markReminderCompleted(userId); break;
            case 4: checkUpcomingReminders(userId); break;
            case 5: editReminder(userId); break;
            case 6: deleteReminder(userId); break;
            case 7: return;
            default: break;
        }
    }
}

void editReminder(int userId)
{
    int id, index = -1;
    char *title = NULL, *date = NULL;
    viewReminders(userId);
    id = readInt("Reminder ID to edit (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    for (int i = 0; i < reminderCount; ++i)
        if (reminders[i].userId == userId && reminders[i].reminderId == id) { index = i; break; }
    if (index < 0) { puts("Reminder not found in your account."); return; }
    title = readLineDynamic("New title (Enter keeps current): ");
    if (title == NULL) return;
    date = readLineDynamic("New date YYYY-MM-DD (Enter keeps current): ");
    if (date == NULL) { free(title); return; }
    if (title[0] != '\0' && strlen(title) >= MAX_TITLE_LEN) {
        puts("Title too long; no changes made."); free(title); free(date); return;
    }
    if (date[0] != '\0' && !isValidDate(date)) {
        puts("Invalid date; no changes made."); free(title); free(date); return;
    }
    if (title[0] != '\0') (void)snprintf(reminders[index].title, sizeof(reminders[index].title), "%s", title);
    if (date[0] != '\0') (void)snprintf(reminders[index].date, sizeof(reminders[index].date), "%s", date);
    saveAllData();
    puts("Reminder updated.");
    free(title); free(date);
}

void deleteReminder(int userId)
{
    int id, index = -1;
    viewReminders(userId);
    id = readInt("Reminder ID to delete (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    for (int i = 0; i < reminderCount; ++i)
        if (reminders[i].userId == userId && reminders[i].reminderId == id) { index = i; break; }
    if (index < 0) { puts("Reminder not found in your account."); return; }
    if (readInt("Delete this reminder? 1. Yes  2. No: ", 1, 2) != 1) { puts("Cancelled."); return; }
    for (int i = index; i < reminderCount - 1; ++i) reminders[i] = reminders[i + 1];
    --reminderCount;
    memset(&reminders[reminderCount], 0, sizeof(reminders[reminderCount]));
    saveAllData();
    puts("Reminder deleted.");
}
