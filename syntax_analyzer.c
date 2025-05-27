/* 
Tran Dai Hiep - 20226081
Tran Pham Minh Duc - 20226077
Nguyen Khoa Ninh - 20226117
*/

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
    LPARENT, RPARENT, LBRACK, RBRACK, PERIOD, COMMA, SEMICOLON, ASSIGN, PERCENT,
    EOFS
} TokenType;

const TokenType keywordTokens[] = {
    BEGIN, CALL, CONST, DO, ELSE, END, FOR, IF, ODD,
    PROCEDURE, PROGRAM, THEN, TO, VAR, WHILE
};

const char* keywords[] = {
    "BEGIN", "CALL", "CONST", "DO", "ELSE", "END", "FOR", "IF", "ODD",
    "PROCEDURE", "PROGRAM", "THEN", "TO", "VAR", "WHILE"
};

typedef struct {
    TokenType type;
    char lexeme[MAX_IDENT_LEN + 1];  // save identifier string
    int numberValue;                 // Save number value for NUMBER
} Token;

TokenType getToken(char *str) {
    int numKeywords = sizeof(keywords) / sizeof(keywords[0]);  // Get number of keywords

    for (int i = 0; i < numKeywords; i++) {
        if (strcmp(str, keywords[i]) == 0) { // Compare input with keyword list
            return keywordTokens[i]; // Return corresponding token
        }
    }
    
    return IDENT; // If not a keyword, return IDENT
}

Token getNextToken(FILE *source) {
    Token token; 
    token.type = NONE;
    token.lexeme[0] = '\0';
    token.numberValue = 0;

    int ch; 

    // 1. Passing whitespace characters
    do {
        ch = fgetc(source);
        if (ch == EOF) {
            token.type = EOFS;
            return token;
        }
    } while (isspace(ch));

    // 2. Read input character
    int i = 0; // index for lexeme

    // 2a. If it's a letter -> IDENT or KEYWORD
    if (isalpha(ch)) {
        char Id[MAX_IDENT_LEN + 1];
        Id[i++] = toupper(ch); 
        token.lexeme[0] = toupper(ch); 

        while (i < MAX_IDENT_LEN) {
            ch = fgetc(source);
            if (isalnum(ch)) { // if character is a number or letter
                Id[i++] = toupper(ch);
                token.lexeme[i - 1] = toupper(ch); 
            } else {
                ungetc(ch, source); // if not a number or letter, push back the character
                break; 
            }
        }

        Id[i] = '\0';
        token.lexeme[i] = '\0';

        if (i == MAX_IDENT_LEN) {
            ch = fgetc(source);
            if (isalnum(ch)) {
                fprintf(stderr, "Lexical Error: Identifier starting with '%s' is too long.\n", token.lexeme);
               
                while(isalnum(ch = fgetc(source)));
                ungetc(ch, source);
                token.type = NONE; // Error: identifier too long
                return token;
            } else {
                ungetc(ch, source); 
            }
        }

        token.type = getToken(Id); // Check if the identifier is a keyword

    // 2b. If it's a number -> NUMBER
    } else if (isdigit(ch)) {
        char numStr[MAX_NUM_LEN + 1]; 
        numStr[i++] = ch;
        token.lexeme[0] = ch;
        token.numberValue = ch - '0';

        while (i < MAX_NUM_LEN) {
            ch = fgetc(source);
            if (isdigit(ch)) {
                numStr[i++] = ch;
                token.lexeme[i - 1] = ch;
                token.numberValue = token.numberValue * 10 + (ch - '0');
            } else {
                ungetc(ch, source); // Push back the character if it's not a digit
                break;
            }
        }

        numStr[i] = '\0';
        token.lexeme[i] = '\0';

        if (i == MAX_NUM_LEN) {
            ch = fgetc(source);
            if (isdigit(ch)) {
                fprintf(stderr, "Lexical Error: Number starting with '%s' is too long.\n", token.lexeme);
                while(isdigit(ch = fgetc(source)));
                ungetc(ch, source);
                token.type = NONE; // Error: number too long
                return token;
            } else {
                ungetc(ch, source); // Push back the character if it's not a digit
            }
        }
        token.type = NUMBER;

    // 2c. If it's a special character -> check for operators and delimiters
    } else {
        token.lexeme[i++] = ch; 
        switch (ch) {
            case '+': token.type = PLUS; break;
            case '-': token.type = MINUS; break;
            case '*': token.type = TIMES; break;
            case '/': token.type = SLASH; break;
            case '%': token.type = PERCENT; break;
            case '=': token.type = EQU; break;
            case '(': token.type = LPARENT; break;
            case ')': token.type = RPARENT; break;
            case '[': token.type = LBRACK; break;
            case ']': token.type = RBRACK; break;
            case ',': token.type = COMMA; break;
            case ';': token.type = SEMICOLON; break;
            case '.': token.type = PERIOD; break;
            case '<':
                ch = fgetc(source); 
                if (ch == '=') {
                    token.type = LEQ;
                    token.lexeme[i++] = ch;
                } else if (ch == '>') {
                    token.type = NEQ;
                    token.lexeme[i++] = ch;
                } else {
                    token.type = LSS;
                    ungetc(ch, source); // Push back if not '=' or '>'
                }
                break;
            case '>':
                ch = fgetc(source); 
                if (ch == '=') {
                    token.type = GEQ;
                    token.lexeme[i++] = ch;
                } else {
                    token.type = GTR;
                    ungetc(ch, source); // Push back if not '='
                }
                break;
            case ':':
                ch = fgetc(source); 
                if (ch == '=') {
                    token.type = ASSIGN;
                    token.lexeme[i++] = ch;
                } else {
                    ungetc(ch, source); // Push back if not '='
                    fprintf(stderr, "Lexical Error: Unexpected character ':'\n");
                    token.type = NONE; // Error: ':' not followed by '='
                }
                break;
            default:
                fprintf(stderr, "Lexical Error: Unknown character '%c'\n", ch);
                token.type = NONE; // Error: unknown character
                break;
        }
        token.lexeme[i] = '\0'; // Null-terminate the lexeme
    }

    return token;
}

Token currentToken;
FILE *inputFile;

// Error handling function
void Error(const char *msg) {
    fprintf(stderr, "Syntax Error: %s\n", msg);
    exit(EXIT_FAILURE);
} 

// Function to consume the current token and get the next one
void consumeToken() {
    currentToken = getNextToken(inputFile);
}

void program();
void block();
void statement();
void expression();
void condition();
void term();
void factor();


void factor() {
    if (currentToken.type == IDENT) {
        consumeToken(); 
        if (currentToken.type == LBRACK) { 
            consumeToken(); 
            expression();    
            if (currentToken.type == RBRACK) { 
                consumeToken(); 
            } else {
                Error("Expected ']' after array index expression");
            }
        }
    } else if (currentToken.type == NUMBER) {
        consumeToken(); 
    } else if (currentToken.type == LPARENT) {
        consumeToken(); 
        expression();  
        if (currentToken.type == RPARENT) { 
            consumeToken(); 
        } else {
            Error("Expected ')' after expression in parentheses");
        }
    } else {
        Error("Invalid factor: Expected Identifier, Number, or '('");
    }
}

void term() {
    factor();
    while (currentToken.type == TIMES || currentToken.type == SLASH || currentToken.type == PERCENT) {
        consumeToken(); 
        factor();
    }
}

void expression() {
    if (currentToken.type == PLUS || currentToken.type == MINUS) {
        consumeToken();
    }
    term();
    while (currentToken.type == PLUS || currentToken.type == MINUS) {
        consumeToken(); 
        term();
    }
}

void condition() {
    if (currentToken.type == ODD) { 
        consumeToken(); 
        expression();
    } else { 
        expression();
        if (currentToken.type == EQU || currentToken.type == NEQ || currentToken.type == LSS ||
            currentToken.type == LEQ || currentToken.type == GTR || currentToken.type == GEQ) {
            consumeToken(); 
            expression();
        } else {
            Error("Expected a relational operator (=, <>, <, <=, >, >=) in condition");
        }
    }
}

void statement() {
    switch (currentToken.type) {
        case IDENT: 
            consumeToken(); 
            if (currentToken.type == LBRACK) { 
                consumeToken(); 
                expression();    
                if (currentToken.type == RBRACK) { 
                    consumeToken(); 
                } else {
                    Error("Expected ']' after array index expression in assignment");
                }
            }
            if (currentToken.type == ASSIGN) {
                consumeToken(); 
            } else {
                Error("Expected ':=' after identifier in assignment statement");
            }
            expression();
            break;

        case CALL:
            consumeToken(); 
            if (currentToken.type == IDENT) { 
                consumeToken(); 
            } else {
                 Error("Expected procedure name after CALL");
            }
            if (currentToken.type == LPARENT) { 
                consumeToken(); 
                expression(); 
                while (currentToken.type == COMMA) {
                    consumeToken(); 
                    expression(); 
                }
                if (currentToken.type == RPARENT) { 
                    consumeToken(); 
                } else {
                    Error("Expected ')' after procedure call arguments");
                }
            }
            break;

        case BEGIN:
            consumeToken(); 
            statement();    
            while (currentToken.type == SEMICOLON) {
                consumeToken();
                statement();       
            }
            if (currentToken.type == END) { 
                consumeToken(); 
            } else {
                Error("Expected 'END' to close 'BEGIN' block");
            }
            break;

        case IF:
            consumeToken(); 
            condition();    
            if (currentToken.type == THEN) { 
                consumeToken(); 
            } else {
                Error("Expected 'THEN' after condition in IF statement");
            }
            statement();  
            if (currentToken.type == ELSE) { 
                consumeToken(); 
                statement();   
            }
            break;

        case WHILE:
            consumeToken(); 
            condition();    
            if (currentToken.type == DO) { 
                consumeToken(); 
            } else {
                Error("Expected 'DO' after condition in WHILE statement");
            }
            statement();   
            break;

        case FOR:
            consumeToken(); 
            if (currentToken.type == IDENT) { 
                consumeToken(); 
            } else {
                Error("Expected identifier for loop variable after FOR");
            }
            if (currentToken.type == ASSIGN) { 
                consumeToken(); 
            } else {
                Error("Expected ':=' after loop variable in FOR statement");
            }
            expression();   
            if (currentToken.type == TO) { 
                consumeToken();
            } else {
                Error("Expected 'TO' after starting value in FOR statement");
            }
            expression();   
            if (currentToken.type == DO) { 
                consumeToken();
            } else {
                Error("Expected 'DO' after ending value in FOR statement");
            }
            statement();   
            break;

        default:
            break;
    }
}

void block() {
    if (currentToken.type == CONST) {
        consumeToken(); 
        do {
            if (currentToken.type == IDENT) {
                consumeToken(); 
            } else {
                Error("Expected identifier in CONST declaration");
            }
            if (currentToken.type == EQU) {
                consumeToken(); 
            } else {
                Error("Expected '=' after constant identifier");
            }
            if (currentToken.type == NUMBER) {
                consumeToken();
            } else {
                Error("Expected number after '=' in CONST declaration");
            }
        } while (currentToken.type == COMMA ? (consumeToken(), 1) : 0); 

        if (currentToken.type == SEMICOLON) {
            consumeToken();
        } else {
            Error("Expected ';' after CONST declarations");
        }
    } 

    if (currentToken.type == VAR) {
        consumeToken(); 
        do {
            if (currentToken.type == IDENT) {
                consumeToken(); 
            } else {
                Error("Expected identifier in VAR declaration");
            }
            if (currentToken.type == LBRACK) {
                consumeToken(); 
                if (currentToken.type == NUMBER) {
                    consumeToken();
                } else {
                    Error("Expected number for array size");
                }
                if (currentToken.type == RBRACK) {
                    consumeToken(); 
                } else {
                    Error("Expected ']' after array size");
                }
            }
        } while (currentToken.type == COMMA ? (consumeToken(), 1) : 0);

        if (currentToken.type == SEMICOLON) {
            consumeToken(); 
        } else {
            Error("Expected ';' after VAR declarations");
        }
    } 

    while (currentToken.type == PROCEDURE) {
        consumeToken(); 

        if (currentToken.type == IDENT) {
            consumeToken(); 
        } else {
            Error("Expected procedure name after PROCEDURE");
        }

        if (currentToken.type == LPARENT) {
            consumeToken(); 
            if (currentToken.type == IDENT || currentToken.type == VAR) {
                if (currentToken.type == VAR) {
                    consumeToken(); 
                }
                if (currentToken.type == IDENT) {
                    consumeToken(); 
                } else {
                    Error("Expected parameter identifier");
                }
                while (currentToken.type == SEMICOLON) {
                    consumeToken(); 
                    if (currentToken.type == VAR) {
                        consumeToken(); 
                    }
                    if (currentToken.type == IDENT) {
                        consumeToken(); 
                    } else {
                        Error("Expected parameter identifier after ';'");
                    }
                }
            }
            if (currentToken.type == RPARENT) {
                consumeToken(); 
            } else {
                Error("Expected ')' to end parameter list");
            }
        }

        if (currentToken.type == SEMICOLON) {
            consumeToken(); 
        } else {
            Error("Expected ';' after procedure header");
        }

        block();

        if (currentToken.type == SEMICOLON) {
            consumeToken(); 
        } else {
            Error("Expected ';' after procedure block");
        }
    } 

    if (currentToken.type == BEGIN) { 
        consumeToken(); 
    } else {
        Error("Expected 'BEGIN' keyword to start the block body after declarations");
    }

    statement();

    while (currentToken.type == SEMICOLON) {
        consumeToken(); 
        statement();    
    }

    if (currentToken.type == END) { 
        consumeToken();
    } else {
        Error("Expected 'END' keyword to close the block body");
    }

} 


void program() {
    if (currentToken.type == PROGRAM) { 
        consumeToken(); 
    } else {
        Error("Program must start with 'PROGRAM' keyword");
    }

    if (currentToken.type == IDENT) { 
        consumeToken(); 
    } else {
        Error("Expected program name (identifier) after 'PROGRAM'");
    }

    if (currentToken.type == SEMICOLON) { 
        consumeToken(); 
    } else {
        Error("Expected ';' after program name");
    }

    block();

    if (currentToken.type == PERIOD) { 
        consumeToken();
    } else {
        Error("Program must end with a '.'");
    }

    if (currentToken.type != EOFS) {
        Error("Unexpected tokens after the final '.'");
    }

    printf("Syntax analysis successful!\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    inputFile = fopen(argv[1], "r");
    if (inputFile == NULL) {
        perror("Error opening source file");
        return EXIT_FAILURE;
    }

    consumeToken();

    program();

    fclose(inputFile);
    return 0;
}