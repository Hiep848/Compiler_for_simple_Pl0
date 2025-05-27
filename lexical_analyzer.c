// gcc lexical_analyzer.c -o lexical_analyzer
// ./lexical_analyzer source.txt listing.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_IDENT_LEN 11
#define MAX_NUM_LEN 6

typedef enum { // Token types in PL/0
    NONE = 0, IDENT, NUMBER,
    BEGIN, CALL, CONST, DO, ELSE, END, FOR, IF, ODD,
    PROCEDURE, PROGRAM, THEN, TO, VAR, WHILE,
    PLUS, MINUS, TIMES, SLASH, EQU, NEQ, LSS, LEQ, GTR, GEQ,
    LPARENT, RPARENT, LBRACK, RBRACK, PERIOD, COMMA, SEMICOLON, ASSIGN, PERCENT
} TokenType;

const TokenType keywordTokens[] = {
    BEGIN, CALL, CONST, DO, ELSE, END, FOR, IF, ODD,
    PROCEDURE, PROGRAM, THEN, TO, VAR, WHILE
};

const char* keywords[] = {
    "BEGIN", "CALL", "CONST", "DO", "ELSE", "END", "FOR", "IF", "ODD",
    "PROCEDURE", "PROGRAM", "THEN", "TO", "VAR", "WHILE"
};

TokenType token;
int Num;
char Id[MAX_IDENT_LEN + 1]; 

char getCh(FILE *file){
    char ch = fgetc(file); // Read a character from the file
    return (ch != EOF && isalpha(ch)) ? toupper(ch) : ch;  // Convert to uppercase if it's a letter
}

TokenType getToken(char *str) {
    int numKeywords = sizeof(keywords) / sizeof(keywords[0]);  // Get number of keywords

    for (int i = 0; i < numKeywords; i++) {
        if (strcmp(str, keywords[i]) == 0) { // Compare input with keyword list
            return keywordTokens[i]; // Return corresponding token
        }
    }
    
    return IDENT; // If not a keyword, return IDENT
}

void lexicalAnalyzer(FILE *source, FILE *destination){
    char ch;
    while ((ch = getCh(source)) == ' ' || ch == '\n' || ch == '\t'){ 
        // Skip whitespace characters
    }
    ungetc(ch, source); // Put back the last read character
    while((ch = getCh(source)) != EOF){
//        printf("Current character: %c\n", ch); // Debugging line
        if (isspace(ch)) { // Ignore whitespace
            continue;
        }
        if (isalpha(ch)) { // Ident or Keywords
            int i = 0;
            do {
               if (i < MAX_IDENT_LEN){
                    Id[i++] = ch;
                    ch = getCh(source);
               } 
               else {
                fprintf(destination, "Error: Identifier too long\n");
                exit(EXIT_FAILURE);
                }
            } while (isalnum(ch));
            
            Id[i] = '\0'; // Null-terminate the identifier string
            ungetc(ch, source); 
            token = getToken(Id);
            if (token == IDENT) {
                fprintf(destination, "IDENT: %s\n", Id); 
            } else {
                fprintf(destination, "TOKEN: %s\n", keywords[token - BEGIN]);
            }
        }

        else if (isdigit(ch)){
            Num = 0;
            int i = 0;
            do {
                if (i < MAX_NUM_LEN) {
                    Num = Num * 10 + (ch - '0'); // Convert char to int
                    ch = getCh(source);
                    i++;
                } else {
                    fprintf(destination, "Error: Number too long\n");
                    exit(EXIT_FAILURE);
                }
            } while (isdigit(ch));
            ungetc(ch, source); // Put back the last read character
            fprintf(destination, "NUMBER: %d\n", Num);
        }

        else {
            switch (ch) {
                case '+': fprintf(destination, "PLUS\n"); break;
                case '-': fprintf(destination, "MINUS\n"); break;
                case '*': fprintf(destination, "TIMES\n"); break;
                case '/': fprintf(destination, "SLASH\n"); break;
                case '=': fprintf(destination, "EQU\n"); break;
                case '<':
                    ch = getCh(source);
                    if (ch == '=') {
                        fprintf(destination, "LEQ\n");
                    } else if (ch == '>') {
                        fprintf(destination, "NEQ\n");
                    } else {
                        ungetc(ch, source); 
                        fprintf(destination, "LSS\n");
                    }
                    break;
                case '>':
                    ch = getCh(source);
                    if (ch == '=') {
                        fprintf(destination, "GEQ\n");
                    } else {
                        ungetc(ch, source); 
                        fprintf(destination, "GTR\n");
                    }
                    break;
                case ':':
                    ch = getCh(source);
                    if (ch == '=') {
                        fprintf(destination, "ASSIGN\n");
                    } else {
                        ungetc(ch, source); 
                        fprintf(destination, "Error: Unknown token \n");
                        exit(EXIT_FAILURE);
                    }
                    break;
                case '(': fprintf(destination, "LPARENT\n"); break;
                case ')': fprintf(destination, "RPARENT\n"); break;
                case '.': fprintf(destination, "PERIOD\n"); break;
                case ',': fprintf(destination, "COMMA\n"); break;
                case ';': fprintf(destination, "SEMICOLON\n"); break;
                case '%': fprintf(destination, "PERCENT\n"); break;
                case '[': fprintf(destination, "LBRACK\n"); break;
                case ']': fprintf(destination, "RBRACK\n"); break;
                default: 
                    fprintf(destination, "Error: Unknown character %c\n", ch);
                    exit(EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <listing_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    FILE *source = fopen(argv[1], "r");
    if (source == NULL) {
        perror("Error opening source file");
        return EXIT_FAILURE;
    }
    FILE *listing = fopen(argv[2], "w");
    if (listing == NULL) {
        perror("Error opening listing file");
        fclose(source);
        return EXIT_FAILURE;
    }

    // Call the lexical analyzer
    lexicalAnalyzer(source, listing); 
    fclose(source); 
    fclose(listing);
    printf("Lexical analysis completed successfully.\n");
    return 0;

}