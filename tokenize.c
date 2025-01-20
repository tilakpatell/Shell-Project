#include <stdio.h>
#include <stdlib.h>

// Declares the tokenize function (assumed to be defined elsewhere).
char** tokenize(const char* input);

// Main function
int main(int argc, char **argv) {
    char input[255];  // Buffer to store the input string.

    // Reads a line of input from standard input (stdin).
    fgets(input, sizeof(input), stdin);

    // Tokenizes the input string.
    char** tokens = tokenize(input);

    // Iterates through the tokens and prints each one.
    for (int i = 0; tokens[i] != NULL; i++) {
        printf("%s\n", tokens[i]);  
        free(tokens[i]); 
    }

    // Frees the memory allocated for the token list.
    free(tokens);

    return 0;  // Indicates that the program finished successfully.
}
