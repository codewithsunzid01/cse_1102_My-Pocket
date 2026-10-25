#include "budget.h"
#include <ctype.h>

static int equalIgnoreCase(const char *left, const char *right)
{
    while (*left && *right) {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) return 0;
        ++left; ++right;
    }
    return *left == '\0' && *right == '\0';
}

TokenList tokenizeString(const char *input, const char *delimiter)
{
    TokenList result;
    char *copy, *token;
    memset(&result, 0, sizeof(result));
    if (input == NULL || delimiter == NULL || delimiter[0] == '\0') return result;
    copy = (char *)malloc(strlen(input) + 1U);
    if (copy == NULL) return result;
    strcpy(copy, input);
    token = strtok(copy, delimiter);
    while (token != NULL && result.count < MAX_TOKENS) {
        (void)snprintf(result.tokens[result.count], MAX_TOKEN_LEN, "%s", token);
        ++result.count;
        token = strtok(NULL, delimiter);
    }
    free(copy);
    return result;
}

int stringContainsToken(const char *text, const char *token)
{
    size_t textLen, tokenLen;
    if (text == NULL || token == NULL || token[0] == '\0') return 0;
    textLen = strlen(text);
    tokenLen = strlen(token);
    if (tokenLen > textLen) return 0;
    for (size_t i = 0; i + tokenLen <= textLen; ++i) {
        size_t j;
        for (j = 0; j < tokenLen; ++j) {
            if (tolower((unsigned char)text[i + j]) != tolower((unsigned char)token[j])) break;
        }
        if (j == tokenLen) return 1;
    }
    return 0;
}

static int transactionMatchesQuery(const Transaction *t, const TokenList *tokens)
{
    char searchable[MAX_DATE_LEN + MAX_DESC_LEN + MAX_CATEGORY_LEN + 64];
    (void)snprintf(searchable, sizeof(searchable), "%s %s %s %s %d", t->date,
                   transactionTypeToString(t->type), categoryToString(t->category),
                   t->description, t->transactionId);
    for (int j = 0; j < tokens->count; ++j)
        if (!stringContainsToken(searchable, tokens->tokens[j])) return 0;
    return tokens->count > 0;
}

void searchTransactions(int userId)
{
    char *query;
    TokenList tokens;
    int found = 0;
    query = readLineDynamic("Search by date, type, category, description or transaction ID: ");
    if (query == NULL || query[0] == '\0') { free(query); puts("Search cancelled: enter a keyword."); return; }
    tokens = tokenizeString(query, " \t,.-/\n");
    puts("\n========== SEARCH RESULTS ==========");
    for (int i = 0; i < transactionCount; ++i) {
        const Transaction *t = &transactions[i];
        if (t->userId != userId || !transactionMatchesQuery(t, &tokens)) continue;
        printf("#%d | %s | %s | %s | %.2f BDT | %s\n", t->transactionId, t->date,
               transactionTypeToString(t->type), categoryToString(t->category), t->amount, t->description);
        found = 1;
    }
    if (!found) puts("No matching transaction found in your account.");
    free(query);
}

void search_and_tokenization_demo(void)
{
    char *text, *keyword;
    TokenList tokens;
    int found = 0;
    puts("\n========== SEARCH & TOKENIZATION DEMO ==========");
    text = readLineDynamic("Enter a sentence: ");
    if (text == NULL) return;
    tokens = tokenizeString(text, " ,.-\t\n");
    puts("Tokens:");
    for (int i = 0; i < tokens.count; ++i) printf("%d. %s\n", i + 1, tokens.tokens[i]);
    keyword = readLineDynamic("Enter keyword to search: ");
    if (keyword != NULL) {
        for (int i = 0; i < tokens.count; ++i) {
            if (equalIgnoreCase(tokens.tokens[i], keyword)) { found = 1; break; }
        }
        printf("Keyword '%s' %s in the sentence.\n", keyword, found ? "was found" : "was not found");
    }
    free(keyword);
    free(text);
}
