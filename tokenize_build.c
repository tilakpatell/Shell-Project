#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 256          // Maximum number of tokens in a single input
#define MAX_TOKEN_LENGTH 256     // Maximum length of a single token
#define MAX_INPUT_LENGTH 255     // Maximum length of the input string

// Checks if a character is a whitespace
int is_space(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

// Duplicates a string (allocates new memory and copies the input string)
char* duplicate(const char* s) {
    char* copy = malloc(strlen(s) + 1);
    if (copy != NULL) {
        strcpy(copy, s);
    }
    return copy;
}

// Tokenizes the input string into meaningful lexical units (tokens)
char** tokenize(const char* input) {
    char** tokens = malloc(MAX_TOKENS * sizeof(char*));  // Allocate memory for tokens
    int token_found = 0;      // Counter for the number of tokens found
    int in_quotations = 0;    // Flag to track if we are inside quotes
    int in_parentheses = 0;   // Flag to track if we are inside parentheses
    char buffer[MAX_TOKEN_LENGTH];  // Buffer to store the current token being formed
    int buffer_pos = 0;       // Position in the buffer

    // Iterates through the input string
    for (int i = 0; i < MAX_INPUT_LENGTH && input[i] != '\0'; i++) {
        char current_letter = input[i];

        // Handles quotation marks and toggle the in_quotations flag
        if (current_letter == '"' && !in_parentheses) {
            in_quotations = !in_quotations;
            if (!in_quotations) {  // End of a quoted string
                buffer[buffer_pos] = '\0';
                tokens[token_found++] = duplicate(buffer);
                buffer_pos = 0;
            }
            continue;
        }

        // If we are inside quotations, it treat everything as part of the same token
        if (in_quotations) {
            buffer[buffer_pos++] = current_letter;
            continue;
        }

        // Handles whitespace outside of quotes/parentheses by finalizing the current token
        if (is_space(current_letter)) {
            if (buffer_pos > 0) {
                buffer[buffer_pos] = '\0';
                tokens[token_found++] = duplicate(buffer);
                buffer_pos = 0;
            }
            continue;
        }

        // Handles parentheses as separate tokens
        if (current_letter == '(' || current_letter == ')') {
            if (buffer_pos > 0) {
                buffer[buffer_pos] = '\0';
                tokens[token_found++] = duplicate(buffer);
                buffer_pos = 0;
            }
            buffer[0] = current_letter;
            buffer[1] = '\0';
            tokens[token_found++] = duplicate(buffer);
            continue;
        }

        // Handles special characters like <, >, |, ; as separate tokens
        if (strchr("<>|;", current_letter)) {
            if (buffer_pos > 0) {
                buffer[buffer_pos] = '\0';
                tokens[token_found++] = duplicate(buffer);
                buffer_pos = 0;
            }
            buffer[0] = current_letter;
            buffer[1] = '\0';
            tokens[token_found++] = duplicate(buffer);
            continue;
        }

        buffer[buffer_pos++] = current_letter;
    }

    // Finalize any remaining token in the buffer
    if (buffer_pos > 0) {
        buffer[buffer_pos] = '\0';
        tokens[token_found++] = duplicate(buffer);
    }

    tokens[token_found] = NULL;  
    return tokens;
}
