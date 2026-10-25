#include "budget.h"
#include <math.h>
#include <limits.h>

static int nextEventId(void)
{
    int maxId = 0;
    for (int i = 0; i < eventCount; ++i)
        if (events[i].eventId > maxId) maxId = events[i].eventId;
    return maxId + 1;
}

static int nextExpenseId(void)
{
    int maxId = 0;
    for (int i = 0; i < eventCount; ++i)
        for (int j = 0; j < events[i].expenseCount; ++j)
            if (events[i].expenses[j].expenseId > maxId) maxId = events[i].expenses[j].expenseId;
    return maxId + 1;
}

int findEventById(int eventId)
{
    for (int i = 0; i < eventCount; ++i)
        if (events[i].eventId == eventId) return i;
    return -1;
}

static int participantIndex(const Event *event, int userId)
{
    for (int i = 0; i < event->participantCount; ++i)
        if (event->participants[i].userId == userId) return i;
    return -1;
}

void createEvent(int userId)
{
    char *name, *date;
    Event *event;
    if (!isValidUser(userId)) { puts("User not found."); return; }
    if (eventCount >= MAX_EVENTS) { puts("Event storage is full."); return; }
    name = readLineDynamic("Event name: ");
    if (name == NULL || name[0] == '\0' || strlen(name) >= MAX_TITLE_LEN) {
        free(name); puts("Event name is required (maximum 99 characters)."); return;
    }
    date = readLineDynamic("Event date (YYYY-MM-DD, e.g. 2027-03-15): ");
    if (date == NULL || !isValidDate(date)) {
        free(name); free(date); puts("Invalid date. Use a real date in YYYY-MM-DD format (any valid year, e.g. 2027-03-15)."); return;
    }
    event = &events[eventCount++];
    memset(event, 0, sizeof(*event));
    event->eventId = nextEventId();
    event->createdBy = userId;
    (void)snprintf(event->eventName, sizeof(event->eventName), "%s", name);
    (void)snprintf(event->eventDate, sizeof(event->eventDate), "%s", date);
    event->participants[0].userId = userId;
    (void)snprintf(event->participants[0].name, sizeof(event->participants[0].name), "%s", users[findUserById(userId)].name);
    event->participantCount = 1;
    saveAllData();
    printf("Event #%d created. You have been added as the first participant.\n", event->eventId);
    free(name); free(date);
}

void viewEvents(int userId)
{
    int found = 0;
    char displayDate[64];
    puts("\n========== GROUP EVENTS ==========");
    for (int i = 0; i < eventCount; ++i) {
        if (participantIndex(&events[i], userId) < 0 && events[i].createdBy != userId) continue;
        formatDateDisplay(events[i].eventDate, displayDate, sizeof(displayDate));
        printf("ID: %d | %s | Date: %s | Participants: %d | Expenses: %d\n",
               events[i].eventId, events[i].eventName, displayDate,
               events[i].participantCount, events[i].expenseCount);
        found = 1;
    }
    if (!found) puts("No group events found for your account.");
}

void addParticipantToEvent(int eventId)
{
    int index = findEventById(eventId), userId;
    Event *event;
    if (index < 0) { puts("Event not found."); return; }
    event = &events[index];
    if (event->participantCount >= MAX_PARTICIPANTS) { puts("Participant limit reached."); return; }
    userId = readInt("Enter registered user ID to add: ", 1, INT_MAX);
    if (!isValidUser(userId)) { puts("No registered user has that ID."); return; }
    if (participantIndex(event, userId) >= 0) { puts("That user is already participating."); return; }
    event->participants[event->participantCount].userId = userId;
    (void)snprintf(event->participants[event->participantCount].name,
                   sizeof(event->participants[event->participantCount].name), "%s", users[findUserById(userId)].name);
    ++event->participantCount;
    saveAllData();
    puts("Participant added successfully.");
}

void splitExpenseEqual(Event *event, EventExpense *expense)
{
    if (event == NULL || expense == NULL || event->participantCount <= 0) return;
    expense->splitType = EQUAL_SPLIT;
    expense->participantCount = event->participantCount;
    for (int i = 0; i < event->participantCount; ++i) {
        expense->shares[i].userId = event->participants[i].userId;
        expense->shares[i].shareAmount = expense->totalAmount / event->participantCount;
    }
}

void splitExpenseCustom(Event *event, EventExpense *expense)
{
    double total = 0.0;
    if (event == NULL || expense == NULL) return;
    expense->splitType = CUSTOM_SPLIT;
    expense->participantCount = event->participantCount;
    for (int i = 0; i < event->participantCount; ++i) {
        double share = readDouble("Share amount for this participant (BDT): ", 0.0, expense->totalAmount);
        if (!isfinite(share) || share < 0.0 || share > expense->totalAmount) {
            puts("Invalid or incomplete share input. Expense cancelled.");
            expense->participantCount = 0;
            return;
        }
        expense->shares[i].userId = event->participants[i].userId;
        expense->shares[i].shareAmount = share;
        total += share;
    }
    if (!isfinite(total) || fabs(total - expense->totalAmount) > 0.01) {
        puts("Custom shares must add up to the total expense. Expense cancelled.");
        expense->participantCount = 0;
    }
}

void addEventExpense(int eventId)
{
    int index = findEventById(eventId), payer, split;
    char *title, date[MAX_DATE_LEN];
    double amount;
    Event *event;
    EventExpense *expense;
    if (index < 0) { puts("Event not found."); return; }
    event = &events[index];
    if (event->expenseCount >= MAX_EVENT_EXPENSES) { puts("This event has reached its expense limit."); return; }
    if (transactionCount >= MAX_TRANSACTIONS) { puts("Transaction storage is full; event expense was not recorded."); return; }
    if (event->participantCount <= 0) { puts("Add participants before adding expenses."); return; }
    title = readLineDynamic("Expense description: ");
    if (title == NULL || title[0] == '\0' || strlen(title) >= MAX_TITLE_LEN) {
        free(title); puts("Description is required."); return;
    }
    amount = readDouble("Total expense amount (BDT): ", 0.01, 1000000000000.0);
    puts("Who paid? Enter a participant's user ID:");
    for (int i = 0; i < event->participantCount; ++i)
        printf("%d - %s (ID %d)\n", i + 1, event->participants[i].name, event->participants[i].userId);
    payer = readInt("Payer user ID: ", 1, INT_MAX);
    if (participantIndex(event, payer) < 0) { free(title); puts("Payer must be an event participant."); return; }
    expense = &event->expenses[event->expenseCount];
    memset(expense, 0, sizeof(*expense));
    expense->expenseId = nextExpenseId();
    expense->eventId = eventId;
    expense->paidBy = payer;
    expense->totalAmount = amount;
    (void)snprintf(expense->title, sizeof(expense->title), "%s", title);
    getCurrentDate(date);
    (void)snprintf(expense->date, sizeof(expense->date), "%s", date);
    puts("Split: 1. Equal  2. Custom");
    split = readInt("Choose split type: ", 1, 2);
    if (split == 1) splitExpenseEqual(event, expense);
    else splitExpenseCustom(event, expense);
    if (expense->participantCount == 0) { free(title); return; }
    ++event->expenseCount;
    /* Record the payment as a real expense for the payer, so their balance reflects cash paid. */
    addTransaction(payer, EXPENSE, OTHERS, amount, date, title);
    {
        int payerIndex = findUserById(payer);
        if (payerIndex >= 0) users[payerIndex].balance -= amount;
    }
    saveAllData();
    printf("Group expense recorded. %s shares saved for %d participant(s).\n",
           splitTypeToString(expense->splitType), expense->participantCount);
    free(title);
}

void viewEventDetails(int eventId)
{
    int index = findEventById(eventId);
    Event *event;
    if (index < 0) { puts("Event not found."); return; }
    event = &events[index];
    { char displayDate[64]; formatDateDisplay(event->eventDate, displayDate, sizeof(displayDate));
      printf("\nEvent #%d: %s | Date: %s\n", event->eventId, event->eventName, displayDate); }
    puts("Participants:");
    for (int i = 0; i < event->participantCount; ++i)
        printf("  %s (ID %d)\n", event->participants[i].name, event->participants[i].userId);
    for (int i = 0; i < event->expenseCount; ++i) {
        EventExpense *expense = &event->expenses[i];
        printf("\nExpense #%d: %s | Total %.2f BDT | Paid by user %d | %s\n",
               expense->expenseId, expense->title, expense->totalAmount, expense->paidBy,
               splitTypeToString(expense->splitType));
        for (int j = 0; j < expense->participantCount; ++j)
            printf("  User %d share: %.2f BDT\n", expense->shares[j].userId, expense->shares[j].shareAmount);
    }
    puts("\nSettlement status (positive = owed to this participant; negative = participant owes):");
    for (int i = 0; i < event->participantCount; ++i) {
        int participantId = event->participants[i].userId;
        printf("  %s (ID %d): %+.2f BDT\n", event->participants[i].name,
               participantId, calculateEventNet(eventId, participantId));
    }
}

double calculateEventNet(int eventId, int userId)
{
    int index = findEventById(eventId);
    double net = 0.0;
    if (index < 0) return 0.0;
    for (int i = 0; i < events[index].expenseCount; ++i) {
        EventExpense *expense = &events[index].expenses[i];
        if (expense->paidBy == userId) net += expense->totalAmount;
        for (int j = 0; j < expense->participantCount; ++j)
            if (expense->shares[j].userId == userId) net -= expense->shares[j].shareAmount;
    }
    /* A positive net means the participant is owed money. Settlements reduce
       the payer's debt and the collector's credit without counting as expenses. */
    for (int i = 0; i < fundingCount; ++i) {
        if (fundings[i].eventId != eventId) continue;
        if (fundings[i].fromUserId == userId) net += fundings[i].amount;
        if (fundings[i].toUserId == userId) net -= fundings[i].amount;
    }
    return net;
}

static int nextFundingId(void)
{
    int maximum = 0;
    for (int i = 0; i < fundingCount; ++i)
        if (fundings[i].fundingId > maximum) maximum = fundings[i].fundingId;
    return maximum + 1;
}

void collectFund(int eventId, int collectorUserId)
{
    int index = findEventById(eventId), payer;
    double amount, payerNet, collectorNet;
    char date[MAX_DATE_LEN];
    if (index < 0) { puts("Event not found."); return; }
    if (participantIndex(&events[index], collectorUserId) < 0) { puts("You must be a participant to collect funds."); return; }
    payer = readInt("User ID paying the fund: ", 1, INT_MAX);
    if (participantIndex(&events[index], payer) < 0 || payer == collectorUserId) { puts("Choose another participant in this event."); return; }
    amount = readDouble("Amount to transfer (BDT): ", 0.01, 1000000000000.0);
    if (!isfinite(amount) || amount <= 0.0) { puts("Invalid transfer amount; no balances were changed."); return; }
    if (fundingCount >= MAX_FUNDINGS) { puts("Funding storage is full."); return; }
    if (transactionCount > MAX_TRANSACTIONS - 2) { puts("Not enough transaction capacity to record both sides of this transfer."); return; }
    if (findUserById(payer) < 0 || findUserById(collectorUserId) < 0) { puts("Participant account could not be found."); return; }
    if (users[findUserById(payer)].balance + 0.000001 < amount) { puts("Payer has insufficient balance."); return; }
    payerNet = calculateEventNet(eventId, payer);
    collectorNet = calculateEventNet(eventId, collectorUserId);
    if (payerNet >= -0.005 || collectorNet <= 0.005) {
        puts("This transfer does not match the event's current settlement: the payer must owe money and the collector must be owed money.");
        return;
    }
    if (amount > -payerNet + 0.005 || amount > collectorNet + 0.005) {
        printf("Transfer exceeds the settlement amount. Payer owes %.2f BDT; collector is owed %.2f BDT.\n", -payerNet, collectorNet);
        return;
    }
    deductFromUser(payer, amount);
    addToUser(collectorUserId, amount);
    getCurrentDate(date);
    fundings[fundingCount].fundingId = nextFundingId();
    fundings[fundingCount].eventId = eventId;
    fundings[fundingCount].fromUserId = payer;
    fundings[fundingCount].toUserId = collectorUserId;
    fundings[fundingCount].amount = amount;
    (void)snprintf(fundings[fundingCount].date, sizeof(fundings[fundingCount].date), "%s", date);
    fundings[fundingCount].type = INTERNAL_TRANSACTION;
    (void)snprintf(fundings[fundingCount].description, sizeof(fundings[fundingCount].description), "Event settlement");
    ++fundingCount;
    recordInternalTransaction(payer, collectorUserId, amount, date, "Event settlement");
    saveAllData();
    puts("Fund transfer recorded.");
}

void deductFromUser(int userId, double amount)
{
    int index = findUserById(userId);
    if (index >= 0 && amount > 0.0) users[index].balance -= amount;
}

void addToUser(int userId, double amount)
{
    int index = findUserById(userId);
    if (index >= 0 && amount > 0.0) users[index].balance += amount;
}

void recordInternalTransaction(int fromUserId, int toUserId, double amount, const char *date, const char *description)
{
    addTransaction(fromUserId, INTERNAL_TRANSACTION, OTHERS, amount, date, description);
    addTransaction(toUserId, INTERNAL_TRANSACTION, OTHERS, amount, date, description);
}

void event_dashboard(int userId)
{
    int choice;
    for (;;) {
        puts("\n========== GROUP EVENT & EXPENSE SHARING ==========");
        puts("1. Create event\n2. View my events\n3. Join/add participant to event\n4. Add event expense\n5. View event details\n6. Settle/transfer funds\n7. Who owes whom? (settlement suggestions)\n8. Edit event\n9. Delete empty event\n10. Remove participant\n11. Edit event expense\n12. Delete event expense\n13. Edit transfer/funding\n14. Delete transfer/funding\n15. Back");
        choice = readInt("Choose: ", 1, 15);
        if (choice == 1) createEvent(userId);
        else if (choice == 2) viewEvents(userId);
        else if (choice == 3) addParticipantToEvent(readInt("Event ID: ", 1, INT_MAX));
        else if (choice == 4) addEventExpense(readInt("Event ID: ", 1, INT_MAX));
        else if (choice == 5) viewEventDetails(readInt("Event ID: ", 1, INT_MAX));
        else if (choice == 6) collectFund(readInt("Event ID: ", 1, INT_MAX), userId);
        else if (choice == 7) showEventSettlementSuggestions(readInt("Event ID: ", 1, INT_MAX));
        else if (choice == 8) editEvent(userId);
        else if (choice == 9) deleteEvent(userId);
        else if (choice == 10) removeParticipantFromEvent(readInt("Event ID: ", 1, INT_MAX));
        else if (choice == 11) editEventExpense(readInt("Event ID: ", 1, INT_MAX));
        else if (choice == 12) deleteEventExpense(readInt("Event ID: ", 1, INT_MAX));
        else if (choice == 13) editFunding(readInt("Event ID: ", 1, INT_MAX));
        else if (choice == 14) deleteFunding(readInt("Event ID: ", 1, INT_MAX));
        else return;
    }
}


void showEventSettlementSuggestions(int eventId)
{
    int index = findEventById(eventId);
    double debt[MAX_PARTICIPANTS], credit[MAX_PARTICIPANTS];
    int any = 0;
    if (index < 0) { puts("Event not found."); return; }
    Event *event = &events[index];
    for (int i = 0; i < event->participantCount; ++i) {
        double net = calculateEventNet(eventId, event->participants[i].userId);
        debt[i] = net < -0.005 ? -net : 0.0;
        credit[i] = net > 0.005 ? net : 0.0;
    }
    printf("\nSettlement suggestions for %s (event #%d):\n", event->eventName, eventId);
    for (int i = 0; i < event->participantCount; ++i) {
        for (int j = 0; j < event->participantCount && debt[i] > 0.005; ++j) {
            double amount;
            if (credit[j] <= 0.005 || i == j) continue;
            amount = debt[i] < credit[j] ? debt[i] : credit[j];
            printf("%s (ID %d) pays %s (ID %d): %.2f BDT\n",
                   event->participants[i].name, event->participants[i].userId,
                   event->participants[j].name, event->participants[j].userId, amount);
            debt[i] -= amount;
            credit[j] -= amount;
            any = 1;
        }
    }
    if (!any) puts("All participants are settled; no payment is currently needed.");
}

void editEvent(int userId)
{
    int id, index = -1;
    char *name = NULL, *date = NULL;
    viewEvents(userId);
    id = readInt("Event ID to edit (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    index = findEventById(id);
    if (index < 0 || events[index].createdBy != userId) { puts("Event not found or only its creator can edit it."); return; }
    name = readLineDynamic("New event name (Enter keeps current): ");
    if (name == NULL) return;
    date = readLineDynamic("New event date YYYY-MM-DD (Enter keeps current): ");
    if (date == NULL) { free(name); return; }
    if (name[0] != '\0' && strlen(name) >= MAX_TITLE_LEN) { puts("Name too long; no changes made."); free(name); free(date); return; }
    if (date[0] != '\0' && !isValidDate(date)) { puts("Invalid date; no changes made."); free(name); free(date); return; }
    if (name[0] != '\0') (void)snprintf(events[index].eventName, sizeof(events[index].eventName), "%s", name);
    if (date[0] != '\0') (void)snprintf(events[index].eventDate, sizeof(events[index].eventDate), "%s", date);
    saveAllData(); puts("Event updated."); free(name); free(date);
}

void deleteEvent(int userId)
{
    int id, index;
    viewEvents(userId);
    id = readInt("Event ID to delete (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    index = findEventById(id);
    if (index < 0 || events[index].createdBy != userId) { puts("Event not found or only its creator can delete it."); return; }
    if (events[index].expenseCount > 0) { puts("Cannot delete an event with recorded expenses. Delete/correct its expenses first so balances and settlements stay accurate."); return; }
    for (int i = 0; i < fundingCount; ++i) if (fundings[i].eventId == id) {
        puts("Cannot delete an event with recorded fund transfers. Correct/delete its transfer records first."); return;
    }
    if (readInt("Delete this empty event? 1. Yes  2. No: ", 1, 2) != 1) { puts("Cancelled."); return; }
    for (int i = index; i < eventCount - 1; ++i) events[i] = events[i + 1];
    --eventCount; memset(&events[eventCount], 0, sizeof(events[eventCount]));
    saveAllData(); puts("Event deleted.");
}

void removeParticipantFromEvent(int eventId)
{
    int index = findEventById(eventId), userId, pindex = -1;
    Event *event;
    if (index < 0) { puts("Event not found."); return; }
    event = &events[index];
    puts("Participants:");
    for (int i = 0; i < event->participantCount; ++i) printf("%s (User ID %d)\n", event->participants[i].name, event->participants[i].userId);
    userId = readInt("Participant user ID to remove (0 to cancel): ", 0, INT_MAX);
    if (userId == 0) return;
    if (userId == event->createdBy) { puts("The event creator cannot be removed."); return; }
    for (int i = 0; i < event->participantCount; ++i) if (event->participants[i].userId == userId) { pindex = i; break; }
    if (pindex < 0) { puts("Participant not found."); return; }
    for (int i = 0; i < event->expenseCount; ++i) {
        EventExpense *e = &event->expenses[i];
        if (e->paidBy == userId) { puts("Cannot remove this participant: they paid an event expense."); return; }
        for (int j = 0; j < e->participantCount; ++j) if (e->shares[j].userId == userId) { puts("Cannot remove this participant: they have an expense share. Edit/delete that expense first."); return; }
    }
    for (int i = 0; i < fundingCount; ++i) if (fundings[i].eventId == eventId && (fundings[i].fromUserId == userId || fundings[i].toUserId == userId)) {
        puts("Cannot remove this participant: they have a recorded settlement."); return;
    }
    if (readInt("Remove this participant? 1. Yes  2. No: ", 1, 2) != 1) { puts("Cancelled."); return; }
    for (int i = pindex; i < event->participantCount - 1; ++i) event->participants[i] = event->participants[i + 1];
    --event->participantCount; memset(&event->participants[event->participantCount], 0, sizeof(event->participants[0]));
    saveAllData(); puts("Participant removed.");
}

static int findLinkedEventExpenseTransaction(const EventExpense *expense)
{
    int found = -1, matches = 0;
    for (int i = 0; i < transactionCount; ++i) {
        Transaction *t = &transactions[i];
        if (t->userId == expense->paidBy && t->type == EXPENSE && t->category == OTHERS &&
            fabs(t->amount - expense->totalAmount) < 0.005 && strcmp(t->date, expense->date) == 0 &&
            strcmp(t->description, expense->title) == 0) { found = i; ++matches; }
    }
    return matches == 1 ? found : -1;
}

void editEventExpense(int eventId)
{
    int ei = findEventById(eventId), id, xi = -1, ti, userId;
    char *title = NULL, *date = NULL;
    double oldAmount, amount;
    EventExpense originalExpense;
    Event *event;
    if (ei < 0) { puts("Event not found."); return; }
    event = &events[ei]; userId = event->createdBy;
    for (int i = 0; i < fundingCount; ++i) if (fundings[i].eventId == eventId) {
        puts("This event has recorded settlements. Edit/delete those transfer records first before changing event expenses."); return;
    }
    if (userId <= 0) { puts("Invalid event creator."); return; }
    viewEventDetails(eventId);
    id = readInt("Expense ID to edit (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    for (int i = 0; i < event->expenseCount; ++i) if (event->expenses[i].expenseId == id) { xi = i; break; }
    if (xi < 0) { puts("Expense not found in this event."); return; }
    ti = findLinkedEventExpenseTransaction(&event->expenses[xi]);
    if (ti < 0) { puts("Could not uniquely match this event expense to its transaction. No changes made to avoid corrupting the balance."); return; }
    originalExpense = event->expenses[xi];
    oldAmount = event->expenses[xi].totalAmount;
    title = readLineDynamic("New expense title (Enter keeps current): ");
    if (title == NULL) return;
    date = readLineDynamic("New expense date YYYY-MM-DD (Enter keeps current): ");
    if (date == NULL) { free(title); return; }
    if (title[0] != '\0' && strlen(title) >= MAX_TITLE_LEN) { puts("Title too long."); free(title); free(date); return; }
    if (date[0] != '\0' && !isValidDate(date)) { puts("Invalid date."); free(title); free(date); return; }
    amount = readDouble("Correct total amount (BDT): ", 0.01, 1000000000000.0);
    event->expenses[xi].totalAmount = amount;
    if (event->expenses[xi].splitType == EQUAL_SPLIT) splitExpenseEqual(event, &event->expenses[xi]);
    else splitExpenseCustom(event, &event->expenses[xi]);
    if (event->expenses[xi].participantCount == 0) { event->expenses[xi] = originalExpense; free(title); free(date); puts("Invalid share split; original expense restored."); return; }
    if (title[0] != '\0') {
        (void)snprintf(event->expenses[xi].title, sizeof(event->expenses[xi].title), "%s", title);
        (void)snprintf(transactions[ti].description, sizeof(transactions[ti].description), "%s", title);
        (void)snprintf(transactions[ti].details.expenseNote, sizeof(transactions[ti].details.expenseNote), "%s", title);
    }
    if (date[0] != '\0') {
        (void)snprintf(event->expenses[xi].date, sizeof(event->expenses[xi].date), "%s", date);
        (void)snprintf(transactions[ti].date, sizeof(transactions[ti].date), "%s", date);
    }
    transactions[ti].amount = amount;
    { int payerIndex = findUserById(event->expenses[xi].paidBy); if (payerIndex >= 0) users[payerIndex].balance += oldAmount - amount; }
    saveAllData(); puts("Event expense updated; linked payer balance and settlement shares were updated.");
    free(title); free(date);
}

void deleteEventExpense(int eventId)
{
    int ei = findEventById(eventId), id, xi = -1, ti;
    Event *event;
    if (ei < 0) { puts("Event not found."); return; }
    event = &events[ei];
    for (int i = 0; i < fundingCount; ++i) if (fundings[i].eventId == eventId) {
        puts("This event has recorded settlements. Delete/reverse those transfers first before deleting an expense."); return;
    }
    viewEventDetails(eventId);
    id = readInt("Expense ID to delete (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    for (int i = 0; i < event->expenseCount; ++i) if (event->expenses[i].expenseId == id) { xi = i; break; }
    if (xi < 0) { puts("Expense not found in this event."); return; }
    ti = findLinkedEventExpenseTransaction(&event->expenses[xi]);
    if (ti < 0) { puts("Could not uniquely match this event expense to its transaction. Nothing deleted."); return; }
    if (readInt("Delete this event expense and reverse the payer's balance? 1. Yes  2. No: ", 1, 2) != 1) { puts("Cancelled."); return; }
    { int payerIndex = findUserById(event->expenses[xi].paidBy); if (payerIndex >= 0) users[payerIndex].balance += event->expenses[xi].totalAmount; }
    for (int i = ti; i < transactionCount - 1; ++i) transactions[i] = transactions[i + 1];
    --transactionCount; memset(&transactions[transactionCount], 0, sizeof(transactions[transactionCount]));
    for (int i = xi; i < event->expenseCount - 1; ++i) event->expenses[i] = event->expenses[i + 1];
    --event->expenseCount; memset(&event->expenses[event->expenseCount], 0, sizeof(event->expenses[0]));
    saveAllData(); puts("Event expense deleted and payer balance reversed.");
}


static int findFundingTransaction(int ownerId, const Funding *f)
{
    int found = -1, matches = 0;
    for (int i = 0; i < transactionCount; ++i) {
        Transaction *t = &transactions[i];
        if (t->userId == ownerId && t->type == INTERNAL_TRANSACTION &&
            fabs(t->amount - f->amount) < 0.005 && strcmp(t->date, f->date) == 0 &&
            strcmp(t->description, f->description) == 0) { found = i; ++matches; }
    }
    return matches == 1 ? found : -1;
}

void editFunding(int eventId)
{
    int fi = -1, id, fromTi, toTi, fromIndex, toIndex;
    char *date = NULL;
    double oldAmount, newAmount;
    for (int i = 0; i < fundingCount; ++i) if (fundings[i].eventId == eventId) {
        printf("Funding ID %d | %d -> %d | %.2f BDT | %s\n", fundings[i].fundingId,
               fundings[i].fromUserId, fundings[i].toUserId, fundings[i].amount, fundings[i].date);
    }
    id = readInt("Funding ID to edit (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    for (int i = 0; i < fundingCount; ++i) if (fundings[i].eventId == eventId && fundings[i].fundingId == id) { fi = i; break; }
    if (fi < 0) { puts("Funding record not found for this event."); return; }
    Funding *f = &fundings[fi];
    fromTi = findFundingTransaction(f->fromUserId, f); toTi = findFundingTransaction(f->toUserId, f);
    if (fromTi < 0 || toTi < 0) { puts("Could not uniquely match both linked transfer transactions; no changes made."); return; }
    fromIndex = findUserById(f->fromUserId); toIndex = findUserById(f->toUserId);
    if (fromIndex < 0 || toIndex < 0) { puts("Transfer account not found."); return; }
    oldAmount = f->amount;
    newAmount = readDouble("Correct transfer amount (BDT): ", 0.01, 1000000000000.0);
    if (newAmount > oldAmount && users[fromIndex].balance + 0.000001 < newAmount - oldAmount) {
        puts("Payer has insufficient balance for the increased transfer."); return;
    }
    date = readLineDynamic("Correct date YYYY-MM-DD (Enter keeps current): ");
    if (date == NULL) return;
    if (date[0] != '\0' && !isValidDate(date)) { puts("Invalid date; no changes made."); free(date); return; }
    users[fromIndex].balance += oldAmount - newAmount;
    users[toIndex].balance += newAmount - oldAmount;
    f->amount = newAmount;
    transactions[fromTi].amount = newAmount; transactions[toTi].amount = newAmount;
    if (date[0] != '\0') {
        (void)snprintf(f->date, sizeof(f->date), "%s", date);
        (void)snprintf(transactions[fromTi].date, sizeof(transactions[fromTi].date), "%s", date);
        (void)snprintf(transactions[toTi].date, sizeof(transactions[toTi].date), "%s", date);
    }
    saveAllData(); puts("Funding record and both account balances updated."); free(date);
}

void deleteFunding(int eventId)
{
    int fi = -1, id, fromTi, toTi, fromIndex, toIndex;
    for (int i = 0; i < fundingCount; ++i) if (fundings[i].eventId == eventId)
        printf("Funding ID %d | %d -> %d | %.2f BDT | %s\n", fundings[i].fundingId,
               fundings[i].fromUserId, fundings[i].toUserId, fundings[i].amount, fundings[i].date);
    id = readInt("Funding ID to delete (0 to cancel): ", 0, INT_MAX);
    if (id == 0) return;
    for (int i = 0; i < fundingCount; ++i) if (fundings[i].eventId == eventId && fundings[i].fundingId == id) { fi = i; break; }
    if (fi < 0) { puts("Funding record not found for this event."); return; }
    Funding f = fundings[fi];
    fromTi = findFundingTransaction(f.fromUserId, &f); toTi = findFundingTransaction(f.toUserId, &f);
    if (fromTi < 0 || toTi < 0) { puts("Could not uniquely match both linked transfer transactions; nothing deleted."); return; }
    fromIndex = findUserById(f.fromUserId); toIndex = findUserById(f.toUserId);
    if (fromIndex < 0 || toIndex < 0) { puts("Transfer account not found."); return; }
    if (readInt("Delete this transfer and reverse both balances? 1. Yes  2. No: ", 1, 2) != 1) { puts("Cancelled."); return; }
    users[fromIndex].balance += f.amount; users[toIndex].balance -= f.amount;
    if (fromTi > toTi) { int tmp = fromTi; fromTi = toTi; toTi = tmp; }
    for (int i = toTi; i < transactionCount - 1; ++i) transactions[i] = transactions[i + 1];
    --transactionCount; memset(&transactions[transactionCount], 0, sizeof(transactions[transactionCount]));
    for (int i = fromTi; i < transactionCount - 1; ++i) transactions[i] = transactions[i + 1];
    --transactionCount; memset(&transactions[transactionCount], 0, sizeof(transactions[transactionCount]));
    for (int i = fi; i < fundingCount - 1; ++i) fundings[i] = fundings[i + 1];
    --fundingCount; memset(&fundings[fundingCount], 0, sizeof(fundings[fundingCount]));
    saveAllData(); puts("Transfer deleted and both balances reversed.");
}
