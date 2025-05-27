/* 
Tran Dai Hiep - 20226081
Tran Pham Minh Duc - 20226077
Nguyen Khoa Ninh - 20226117
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define TAB_WIDTH 4
#define MAX_IDENT_LEN 11
#define MAX_NUM_LEN 6
#define MAX_PARAMS 10
#define MAX_SYMBOLS 1024
#define MAX_NESTING_DEPTH 100 
int scope_stack[MAX_NESTING_DEPTH];
int scope_stack_ptr = -1; 

bool compilationErrorOccurred = false;


typedef enum { KIND_CONST, KIND_VAR, KIND_PROC } ObjectKind;
typedef enum { TYPE_NONE, TYPE_INTEGER, TYPE_ARRAY, TYPE_ERROR } DataType;

const char* DataTypeStrings[] = {
    "NONE",
    "INTEGER",
    "ARRAY",
    "TYPE_ERROR"
};
const char* datatype_to_string(DataType type) {
    if (type >= TYPE_NONE && type <= TYPE_ERROR) { 
        return DataTypeStrings[type];
    }
    return "UNKNOWN_DATATYPE";
}

typedef struct {
    DataType type;
    int value; // For constants
    bool isConst; // For constants
} SemanticProperties;

typedef struct {
    char name[MAX_IDENT_LEN + 1];
    ObjectKind kind;
    DataType type;     // Data type of the identifier
    DataType elementType; // Type of elements in case of array
    int value;         // For constants
    int size;          // For arrays
    int level;         // Scope level
    int address;       // Address or offset
    int numParams;
    DataType formalParamTypes[MAX_PARAMS];
} Symbol;

Symbol symbolTable[MAX_SYMBOLS];
int symbolCount = 0;
int currentLevel = 0;

void Enter(char *Id, ObjectKind kind, DataType type, int value, int size) {
    if (symbolCount >= MAX_SYMBOLS) {
        fprintf(stderr, "Symbol table overflow\n");
        exit(EXIT_FAILURE);
    }
    strncpy(symbolTable[symbolCount].name, Id, MAX_IDENT_LEN);
    symbolTable[symbolCount].name[MAX_IDENT_LEN] = '\0';
    symbolTable[symbolCount].kind = kind;
    symbolTable[symbolCount].type = type;
    symbolTable[symbolCount].level = currentLevel;
    symbolTable[symbolCount].address = symbolCount; 
    symbolTable[symbolCount].value = value;
    symbolTable[symbolCount].size = size;

    if (type == TYPE_ARRAY) {
        symbolTable[symbolCount].elementType = TYPE_INTEGER;
    } else {
        symbolTable[symbolCount].elementType = TYPE_NONE;
    }

    symbolCount++;
}

int Location(char *Id) {
    int i;
    int sptr_loop; 
    for (sptr_loop = scope_stack_ptr; sptr_loop >= 0; sptr_loop--) {
        int levelOfScopeBeingSearched = sptr_loop;
        int scopeStartIndex = scope_stack[sptr_loop];
        int scopeEndIndex;

        if (levelOfScopeBeingSearched == 0) {
            scopeStartIndex = 0;
        } else {
            scopeStartIndex = scope_stack[sptr_loop];
        }

        if (sptr_loop == scope_stack_ptr) {
            scopeEndIndex = symbolCount - 1;
        } else {
            scopeEndIndex = scope_stack[sptr_loop + 1] - 1;
        }

        for (i = scopeEndIndex; i >= scopeStartIndex; i--) {
            if (symbolTable[i].level == levelOfScopeBeingSearched && strcmp(symbolTable[i].name, Id) == 0) {
                return i + 1;
            }
        }
    }
    return 0; 
}

int checkIdent(char *Id) {
    if (scope_stack_ptr < 0) {
        return 0; 
    }
    int currentScopeStartIndex = (scope_stack_ptr >= 0) ? scope_stack[scope_stack_ptr] : 0;
    for (int i = symbolCount - 1; i >= currentScopeStartIndex; i--) {
        if (strcmp(symbolTable[i].name, Id) == 0 && symbolTable[i].level == currentLevel) {
            return 1;
        }
    }
    return 0;
}

ObjectKind getKind(int idx) {
    if (idx > 0 && idx <= symbolCount) {
        return symbolTable[idx - 1].kind;
    }
    return -1; // Invalid index
}

typedef enum { // Token types in PL/0
    NONE = 0, IDENT, NUMBER,
    BEGIN, CALL, CONST, DO, ELSE, END, FOR, IF, ODD,
    PROCEDURE, PROGRAM, THEN, TO, VAR, WHILE,
    PLUS, MINUS, TIMES, SLASH, EQU, NEQ, LSS, LEQ, GTR, GEQ,
    LPARENT, RPARENT, LBRACK, RBRACK, PERIOD, COMMA, SEMICOLON, ASSIGN, PERCENT,
    EOFS
} TokenType;

const char* TokenTypeStrings[] = {
    "NONE", "IDENT", "NUMBER",
    "BEGIN", "CALL", "CONST", "DO", "ELSE", "END", "FOR", "IF", "ODD",
    "PROCEDURE", "PROGRAM", "THEN", "TO", "VAR", "WHILE",
    "PLUS", "MINUS", "TIMES", "SLASH", "EQU", "NEQ", "LSS", "LEQ", "GTR", "GEQ",
    "LPARENT", "RPARENT", "LBRACK", "RBRACK", "PERIOD", "COMMA", "SEMICOLON", "ASSIGN", "PERCENT",
    "EOFS"
};
const char* token_to_string(TokenType type) {
    if (type >= NONE && type <= EOFS) { 
        return TokenTypeStrings[type];
    }
    return "UNKNOWN_TOKEN"; 
}
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
    int line;
    int col;
} Token;

const char* filename = NULL;
static int global_currentLine = 1;
static int global_currentCol = 1;
Token previousToken;

bool isStartOfStatement(TokenType type) {
    return type == IDENT || type == CALL || type == BEGIN ||
           type == IF || type == WHILE || type == FOR;
}

void showErrorContext(int line, int col, const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return;
    char buf[512];
    int currentLine = 1;
    while (fgets(buf, sizeof(buf), file)) {
        if (currentLine == line) {
            fprintf(stderr, "%s", buf);
            for (int i = 1; i < col; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\n");
            break;
        }
        currentLine++;
    }
    fclose(file);
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

Token getNextToken(FILE *source) {
    Token token = {NONE, "", 0, 0, 0};
    int ch; 
    // 1. Passing whitespace characters
    do {
        ch = fgetc(source);
        if (ch == EOF) {
            token.type = EOFS;
            token.line = global_currentLine;
            token.col = global_currentCol + 1;
            return token;
        }
        if (ch == '\t'){
            global_currentCol += TAB_WIDTH ; // Adjust for tab width
        }
        else if (ch == '\n') {
            global_currentLine++;
            global_currentCol = 1;
        } else {
            global_currentCol++;
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
                global_currentCol++;
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
                fprintf(stderr, "Lexical Error at Line %d, Column %d: Identifier '%s' too long\n", global_currentLine, global_currentCol, token.lexeme);
                showErrorContext(global_currentCol, global_currentLine, filename);
                do {
                    ch = fgetc(source);
                    global_currentCol++;
                } while (isalnum(ch));

                ungetc(ch, source);
                token.type = NONE; // Error: identifier too long
                return token;
            } else {
                ungetc(ch, source); 
            }
        }

        token.type = getToken(Id); // Check if the identifier is a keyword
        token.line = global_currentLine;
        token.col = global_currentCol;

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
                global_currentCol++;
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
                fprintf(stderr, "Lexical Error at Line %d, Column %d: Number '%s' too long\n", global_currentLine, global_currentCol, token.lexeme);
                showErrorContext(global_currentLine, global_currentCol, filename);
                do {
                    ch = fgetc(source);
                    global_currentCol++;
                } while (isdigit(ch));
                ungetc(ch, source);
                token.type = NONE; // Error: number too long
                return token;
            } else {
                ungetc(ch, source); // Push back the character if it's not a digit
            }
        }
        token.type = NUMBER;
        token.line = global_currentLine;
        token.col = global_currentCol;

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
                    global_currentCol++;
                    token.type = LEQ;
                    token.lexeme[i++] = ch;
                } else if (ch == '>') {
                    global_currentCol++;
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
                    global_currentCol++;
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
                    global_currentCol++;
                } else {
                    ungetc(ch, source); // Push back if not '='
                    fprintf(stderr, "Lexical Error at Line %d, Column %d: Unexpected character ':'\n", global_currentLine, global_currentCol);
                    showErrorContext(global_currentLine, global_currentCol, filename);
                    token.type = NONE; // Error: ':' not followed by '='
                    return token;
                }
                break;
            default:
                fprintf(stderr, "Lexical Error at Line %d, Column %d: Unknown character '%c'\n", global_currentLine, global_currentCol, ch);
                showErrorContext(global_currentLine, global_currentCol, filename);
                token.type = NONE; // Error: unknown character
                return token;
        }
        token.lexeme[i] = '\0'; // Null-terminate the lexeme
        token.line = global_currentLine;
        token.col = global_currentCol;
    }

    return token;
}

Token currentToken;
FILE *inputFile;

const char* token_to_string(TokenType type);
const char* datatype_to_string(DataType type);

void addError(Token t, const char *msg) {
    compilationErrorOccurred = true;
    fprintf(stderr, "Error at Line %d, Column %d (near token '%s'): %s\n",
            t.line, t.col, t.lexeme, msg);
    if (filename) {
        showErrorContext(t.line, t.col, filename);
    }
}
// Error handling function
void Error(Token t, const char *msg) {
    fprintf(stderr, "Syntax Error at Line %d, Column %d (near token '%s'): %s\n",
            t.line, t.col, t.lexeme, msg);
    showErrorContext(t.line, t.col, filename);
    exit(EXIT_FAILURE);
}

// Function to consume the current token and get the next one
void consumeToken() {
    previousToken = currentToken;
    currentToken = getNextToken(inputFile);
}

void program();
void block();
void statement();
SemanticProperties expression();
SemanticProperties condition();
SemanticProperties term();
SemanticProperties factor();

void compileDeclareVariable(void) {
    Token identToken;
    if (currentToken.type == IDENT) {
        identToken = currentToken;
        
        if (checkIdent(currentToken.lexeme) != 0) {
            Error(currentToken, "Variable name already declared in this scope");
            return;
        }
        consumeToken();
        if (currentToken.type == LBRACK) {
            // Array declaration handling (example: VAR arr[10];)
            consumeToken();
            if (currentToken.type == NUMBER) {
                int arraySize = currentToken.numberValue;
                consumeToken();
                if (currentToken.type == RBRACK) {
                    consumeToken();
                    Enter(identToken.lexeme, KIND_VAR, TYPE_ARRAY, 0, arraySize);
                } else {
                    Error(currentToken, "Expected ']' after array size");
                }
            } else {
                Error(currentToken, "Expected array size after '['");
            }
        } else {
            Enter(identToken.lexeme, KIND_VAR, TYPE_INTEGER, 0, 0);
        }
    } else {
        Error(currentToken, "Missing variable name in declaration");
    }
}

void compileDeclareConstant(void){
    Token identToken;
    int constValue;
    if (currentToken.type == IDENT) {
        identToken = currentToken;
        if (checkIdent(currentToken.lexeme) == 0) {
            consumeToken();
            if (currentToken.type == EQU) {
                consumeToken();
                if (currentToken.type == NUMBER) {
                    constValue = currentToken.numberValue;
                    consumeToken();
                    Enter(identToken.lexeme, KIND_CONST, TYPE_INTEGER, constValue, 0);
                } else {
                    Error(currentToken, "Expected constant value after '='");
                }
            }
        } else {
            Error(currentToken, "Constant name already declared in this scope");
        }
    } else {
        Error(currentToken, "Missing constant name in declaration");
    }
}

void compileDeclareProcedure(void) {
    Token procIdentToken;
    Symbol* currentProcedureSymbol = NULL;
    int previousLevel = currentLevel;

    if (currentToken.type == IDENT) {
        procIdentToken = currentToken;
        if (checkIdent(currentToken.lexeme) == 0) {
            Enter(currentToken.lexeme, KIND_PROC, TYPE_NONE, 0, 0);
            if (symbolCount > 0) {
                currentProcedureSymbol = &symbolTable[symbolCount - 1];
            } else {
                Error(procIdentToken, "Failed to enter procedure in symbol table");
                return;
            }
            consumeToken();
            currentLevel++;
            scope_stack_ptr++;
            if (scope_stack_ptr >= MAX_NESTING_DEPTH) {
                fprintf(stderr, "Critical Error: Maximum nesting depth exceeded for procedure %s.\n", procIdentToken.lexeme);
                currentLevel = previousLevel;
                scope_stack_ptr--; 
                exit(EXIT_FAILURE);
            }
            scope_stack[scope_stack_ptr] = symbolCount;
        } else {
            Error(currentToken, "Procedure name already declared in this scope");
        }
    } else {
        Error(currentToken, "Missing procedure name in declaration");
    }

    if (currentToken.type == LPARENT) {
        consumeToken();
        if (currentToken.type == VAR || currentToken.type == IDENT) { 
            do {
                bool isVarParam = false; 
                if (currentToken.type == VAR) {
                    isVarParam = true;
                    consumeToken();
                }

                if (currentToken.type == IDENT) {
                    Token paramToken = currentToken;
                    if (checkIdent(paramToken.lexeme) != 0) {
                        Error(paramToken, "Parameter name already declared in this procedure's scope");
                    } else {
                        Enter(paramToken.lexeme, KIND_VAR, TYPE_INTEGER, 0, 0);

                        if (currentProcedureSymbol != NULL) {
                            if (currentProcedureSymbol->numParams < MAX_PARAMS) {
                                currentProcedureSymbol->formalParamTypes[currentProcedureSymbol->numParams] = TYPE_INTEGER;
                            } else {
                                Error(paramToken, "Too many parameters for procedure.");
                            }
                            currentProcedureSymbol->numParams++;
                        }
                    }
                    consumeToken(); 
                } else {
                    Error(currentToken, "Expected parameter name (identifier).");
                    currentLevel = previousLevel;
                    scope_stack_ptr--;
                    return;
                }

                if (currentToken.type == SEMICOLON) { 
                    consumeToken();
                    if (currentToken.type != IDENT && currentToken.type != VAR) {
                        Error(currentToken, "Expected parameter declaration after ';'.");
                        currentLevel = previousLevel;
                        scope_stack_ptr--;
                        return;
                    }
                } else if (currentToken.type != RPARENT) {
                    Error(currentToken, "Expected ';' or ')' in parameter list.");
                    currentLevel = previousLevel;
                    scope_stack_ptr--; 
                    return;
                }
            } while (currentToken.type != RPARENT);
            if (currentToken.type == RPARENT) {
            consumeToken(); 
            }  
        } else {
            Error(currentToken, "Expected parameter declaration (IDENT or VAR) after '('. An empty parameter list '()' is not allowed.");
            currentLevel = previousLevel;
            scope_stack_ptr--;
            return;
        }
    }
    if (currentToken.type == SEMICOLON) {
        consumeToken();
    } else {
        addError(previousToken, "Expected ';' after procedure header.");
        compilationErrorOccurred = true;
    }
    block();
    currentLevel = previousLevel;
    scope_stack_ptr--;  
}

SemanticProperties factor() {
    SemanticProperties result;
    result.type = TYPE_NONE; // Default type
    result.isConst = false; // Default is not constant

    Token identToken;

    if (currentToken.type == IDENT) {
        identToken = currentToken;
        int p = Location(currentToken.lexeme);
            if (p == 0) {
                Error(currentToken, "Variable not declared before use");
            }
            Symbol* sym = &symbolTable[p - 1];
            consumeToken(); 

            if (sym->kind == KIND_CONST){
                result.type = TYPE_INTEGER;
                result.value = sym->value;
                result.isConst = true;
                if (currentToken.type == LBRACK) {
                    Error(identToken, "Constant is not an array, cannot use subscript.");       
                }
            } else if (sym->kind == KIND_VAR) {
                if (sym->type == TYPE_INTEGER){
                    result.type = TYPE_INTEGER;
                    if (currentToken.type == LBRACK) {
                        Error(identToken, "Variable is not an array (it's an INTEGER), cannot use subscript.");
                    }
                } else if (sym->type == TYPE_ARRAY){

                    result.isConst = false;

                    if (currentToken.type == LBRACK){
                        consumeToken();
                        SemanticProperties indexProps = expression();

                        if (indexProps.type != TYPE_INTEGER) {
                            Error(currentToken, "Array index must be an integer expression");
                            result.type = TYPE_ERROR;
                    } else {
                        if (indexProps.isConst) {
                            int indexValue = indexProps.value;
                            if (indexValue < 0 || indexValue >= sym->size) {
                                char msg[150];
                                sprintf(msg, "Semantic error: Array index [%d] for array '%s' is out of bounds (size: %d, valid indices: 0..%d).",
                                        indexValue, sym->name, sym->size, sym->size - 1);
                                Error(identToken, msg); 
                                result.type = TYPE_ERROR; 
                            }
                        }
                    }
                    if (result.type != TYPE_ERROR) {
                        result.type = sym->elementType;
                    }
                    if (currentToken.type == RBRACK) {
                        consumeToken();
                    } else {
                        Error(currentToken, "Expected ']' after array index expression");
                        result.type = TYPE_ERROR;
                    }  
                } else{
                    Error(identToken, "Cannot use an entire array in this expression context");
                    result.type = TYPE_ERROR;
                }
            } else{
                Error(identToken, "Identifier is not a variable or constant");
                result.type = TYPE_ERROR;
            }
        }  
    } else if (currentToken.type == NUMBER) {
        result.type = TYPE_INTEGER;
        result.value = currentToken.numberValue;
        result.isConst = true;
        consumeToken(); 
    } else if (currentToken.type == LPARENT) {
        consumeToken(); 
        result = expression();  
        if (currentToken.type == RPARENT) { 
            consumeToken(); 
        } else {
            Error(previousToken, "Expected ')' after expression in parentheses");
            result.type = TYPE_ERROR;
            result.isConst = false;
        }
    } else {
        Error(previousToken, "Invalid factor: Expected Identifier, Number, or '('");
        result.type = TYPE_ERROR;
        result.isConst = false;
    }

    return result;
}

SemanticProperties term() {
    SemanticProperties leftProps = factor();
    while (currentToken.type == TIMES || currentToken.type == SLASH || currentToken.type == PERCENT) {
        Token operatorToken = currentToken;
        consumeToken();
        SemanticProperties rightProps = factor();

        if (leftProps.type != TYPE_INTEGER || rightProps.type != TYPE_INTEGER) {
            if (leftProps.type != TYPE_ERROR && rightProps.type != TYPE_ERROR) { 
                char msg[150];
                sprintf(msg, "Type mismatch for operator '%s'. Both operands must be INTEGER, but found '%s' and '%s'.",
                        token_to_string(operatorToken.type),
                        datatype_to_string(leftProps.type),
                        datatype_to_string(rightProps.type));
                Error(operatorToken, msg);
            }
            leftProps.type = TYPE_ERROR;
            leftProps.isConst = false;
        } else { 
            leftProps.type = TYPE_INTEGER; 

            bool compileTimeErrorOccurred = false;

            if (operatorToken.type == SLASH) {
                if (rightProps.isConst && rightProps.value == 0) {
                    Error(operatorToken, "Semantic error: Division by constant zero.");
                    leftProps.type = TYPE_ERROR;
                    leftProps.isConst = false;
                    compileTimeErrorOccurred = true;
                }
            } else if (operatorToken.type == PERCENT) {
                if (rightProps.isConst && rightProps.value == 0) {
                    Error(operatorToken, "Semantic error: Modulo by constant zero.");
                    leftProps.type = TYPE_ERROR;
                    leftProps.isConst = false;
                    compileTimeErrorOccurred = true;
                }
            }

           
            if (!compileTimeErrorOccurred) {
                if (leftProps.isConst && rightProps.isConst) {
                    leftProps.isConst = true;
                    switch (operatorToken.type) {
                        case TIMES:
                            leftProps.value = leftProps.value * rightProps.value;
                            break;
                        case SLASH:
                            leftProps.value = leftProps.value / rightProps.value;
                            break;
                        case PERCENT:
                            leftProps.value = leftProps.value % rightProps.value;
                            break;
                        default:
                            break;
                    }
                } else {
                    leftProps.isConst = false;
                }
            }
        }
        if (rightProps.type == TYPE_ERROR && leftProps.type != TYPE_ERROR) {
             leftProps.type = TYPE_ERROR;
             leftProps.isConst = false;
        }
    }
    return leftProps;
}

SemanticProperties expression() {
    SemanticProperties resultProps;
    TokenType unaryOperator = NONE;
    if (currentToken.type == PLUS || currentToken.type == MINUS) {
        unaryOperator = currentToken.type;
        consumeToken();
    }
    resultProps = term();
    if (unaryOperator != NONE) {
        if (resultProps.type != TYPE_INTEGER) {
            if (resultProps.type != TYPE_ERROR) {
                char msg[150];
                sprintf(msg, "Unary operator '%s' can only be applied to an INTEGER, but found '%s'.",
                        token_to_string(unaryOperator),
                        datatype_to_string(resultProps.type));
                Error(previousToken, msg); 
                resultProps.type = TYPE_ERROR;
            }
            resultProps.isConst = false; 
        } else {
            if (resultProps.isConst && unaryOperator == MINUS) {
                resultProps.value = -resultProps.value;
            }
        }
    }
    while (currentToken.type == PLUS || currentToken.type == MINUS) {
        Token operatorToken = currentToken;
        consumeToken(); 
        SemanticProperties rightProps = term();
        if (resultProps.type != TYPE_INTEGER || rightProps.type != TYPE_INTEGER) {
            if (resultProps.type != TYPE_ERROR && rightProps.type != TYPE_ERROR) {
                char msg[150];
                sprintf(msg, "Type mismatch for binary operator '%s'. Both operands must be INTEGER, but found '%s' and '%s'.",
                        token_to_string(operatorToken.type),
                        datatype_to_string(resultProps.type),
                        datatype_to_string(rightProps.type));
                Error(operatorToken, msg);
            }
            resultProps.type = TYPE_ERROR;
            resultProps.isConst = false;
        } else {
            resultProps.type = TYPE_INTEGER; 

            if (resultProps.isConst && rightProps.isConst) {

                resultProps.isConst = true;
                if (operatorToken.type == PLUS) {
                    resultProps.value = resultProps.value + rightProps.value;
                } else { // MINUS
                    resultProps.value = resultProps.value - rightProps.value;
                }
            } else {
                resultProps.isConst = false;
            }
        }

        if (rightProps.type == TYPE_ERROR) {
            resultProps.type = TYPE_ERROR;
            resultProps.isConst = false;
        }
    }

    return resultProps;
}

SemanticProperties condition() {
    SemanticProperties result;
    result.type = TYPE_NONE; 
    result.isConst = false;  
    result.value = 0; 
    Token errorReportingToken;
    if (currentToken.type == ODD) { 
        errorReportingToken = currentToken;
        consumeToken(); 
        SemanticProperties exprProps = expression();
        if (exprProps.type == TYPE_ERROR) {
            result.type = TYPE_ERROR; 
            return result;
        }
        if (exprProps.type != TYPE_INTEGER) {
            result.type = TYPE_ERROR;
            char msg[150];
            sprintf(msg, "Operand for ODD must be an INTEGER expression, but found '%s'.", datatype_to_string(exprProps.type));
            Error(errorReportingToken, msg);
        }
    } else { 
        SemanticProperties leftExprProps = expression();
        if (currentToken.type == EQU || currentToken.type == NEQ || currentToken.type == LSS ||
            currentToken.type == LEQ || currentToken.type == GTR || currentToken.type == GEQ) {
            Token relOpToken = currentToken; 
            consumeToken(); 
            SemanticProperties rightExprProps = expression();
            if (leftExprProps.type == TYPE_ERROR || rightExprProps.type == TYPE_ERROR) {
                result.type = TYPE_ERROR;
                return result;
            }

            if (leftExprProps.type != TYPE_INTEGER || rightExprProps.type != TYPE_INTEGER) {
                result.type = TYPE_ERROR;
                char msg[200];
                sprintf(msg, "Type mismatch for relational operator '%s'. Both operands must be INTEGER, but found '%s' and '%s'.",
                        token_to_string(relOpToken.type),
                        datatype_to_string(leftExprProps.type),
                        datatype_to_string(rightExprProps.type));
                Error(relOpToken, msg); 
            }
        } else {
            result.type = TYPE_ERROR;
            Error(previousToken, "Expected a relational operator (=, <>, <, <=, >, >=) in condition");
        }
    }
    return result;
}

void statement() {
    Token identToken; 
    int loc;
    Symbol* sym;
    SemanticProperties props, props2, indexProps;

    switch (currentToken.type) {
        case IDENT: 
            identToken = currentToken;
            loc = Location(identToken.lexeme);

            if (loc == 0) {
                Error(identToken, "Identifier not declared (used in assignment)");
                return;
            }
            sym = &symbolTable[loc - 1];

            if (sym->kind != KIND_VAR) {
                Error(identToken, "Identifier on the left side of assignment must be a variable.");
                return;
            }

            consumeToken(); 

            if (currentToken.type == LBRACK) { 
                if (sym->type != TYPE_ARRAY) {
                    Error(identToken, "Identifier is not an array, cannot use subscript.");
                    return;
                }
                consumeToken(); 
                indexProps = expression(); 

                if (indexProps.type != TYPE_INTEGER) {
                    Error(currentToken, "Array index must be an integer expression.");
                }

                if (currentToken.type == RBRACK) {
                    consumeToken(); 
                } else {
                    Error(currentToken, "Expected ']' after array index.");
                    return;
                }

                if (currentToken.type == ASSIGN) {
                    consumeToken(); 
                    props = expression(); 

                    if (sym->elementType != props.type && props.type != TYPE_ERROR) {
                        char msg[150];
                        sprintf(msg, "Type mismatch in assignment to array element.",sym->name);
                        Error(identToken, msg); 
                    }
                } else {
                    Error(currentToken, "Expected ':=' after array element access in assignment.");
                }

            } else { 
                if (sym->type == TYPE_ARRAY) {
                    Error(identToken, "Cannot assign to an entire array. Must specify an index or use a scalar variable.");

                    return;
                }
                if (currentToken.type == ASSIGN) {
                    consumeToken(); 
                    props = expression(); 

                    if (sym->type != props.type && props.type != TYPE_ERROR) {
                        char msg[150];
                        sprintf(msg, "Type mismatch in assignment.",
                                sym->name);
                        Error(identToken, msg);
                    }
                } else {
                    Error(currentToken, "Expected ':=' after variable name in assignment.");
                }
            }
            break;

        case CALL:
            consumeToken(); 
            if (currentToken.type == IDENT) {
                identToken = currentToken;
                loc = Location(identToken.lexeme);
                if (loc == 0) {
                    Error(identToken, "Procedure not declared.");
                    return;
                }
                sym = &symbolTable[loc - 1];
                if (sym->kind != KIND_PROC) {
                    Error(identToken, "Identifier is not a procedure.");
                    return;
                }
                consumeToken(); 

                int actualParamCount = 0;
                SemanticProperties actualParamProps[MAX_PARAMS]; 

                if (currentToken.type == LPARENT) {
                    consumeToken(); 
                    if (currentToken.type != RPARENT) { 
                        do {
                            if (actualParamCount > 0) { 
                                if (currentToken.type == COMMA) {
                                    consumeToken(); 
                                } else {
                                    Error(currentToken, "Expected ',' or ')' in procedure call arguments.");
                                    return;
                                }
                            }
                            if (actualParamCount < MAX_PARAMS) {
                                actualParamProps[actualParamCount] = expression();
                            } else {
                                Error(currentToken, "Too many arguments in procedure call (exceeds internal limit).");
                                expression();
                            }
                            actualParamCount++;
                        } while (currentToken.type == COMMA);
                    }
                    if (currentToken.type == RPARENT) {
                        consumeToken(); 
                    } else {
                        Error(currentToken, "Expected ')' after procedure call arguments.");
                        return;
                    }
                } 
                if (actualParamCount != sym->numParams) {
                    char msg[100];
                    sprintf(msg, "Incorrect number of arguments for procedure '%s'. Expected %d, got %d.",
                            sym->name, sym->numParams, actualParamCount);
                    Error(identToken, msg);
                } else {
                    for (int i = 0; i < actualParamCount; i++) {
                        if (actualParamProps[i].type != sym->formalParamTypes[i] && actualParamProps[i].type != TYPE_ERROR) {
                            char msg[150];
                            sprintf(msg, "Type mismatch for argument %d of procedure '%s'. Expected '%s', got '%s'.",
                                    i + 1, sym->name,
                                    datatype_to_string(sym->formalParamTypes[i]),
                                    datatype_to_string(actualParamProps[i].type));
                            Error(identToken, msg);
                        }
                    }
                }
            } else {
                Error(currentToken, "Expected procedure name after CALL.");
            }
            break;

        case BEGIN:
            consumeToken();
            if (isStartOfStatement(currentToken.type)) {
                statement(); 
            } else if (currentToken.type != END) {
                addError(currentToken, "Expected a statement or END after BEGIN.");
            }
            while (currentToken.type == SEMICOLON || isStartOfStatement(currentToken.type)) {
                Token tokenForMissingSemicolonError = currentToken;

                if (currentToken.type == SEMICOLON) {
                    consumeToken(); 
                    if (!isStartOfStatement(currentToken.type)) {
                        if (currentToken.type == END) {
                            break; 
                        }
                        addError(currentToken, "Expected a statement or END after ';'.");
                        break;
                    }
                } else {
                    addError(tokenForMissingSemicolonError, "Missing ';' before this statement.");
                }
                if (isStartOfStatement(currentToken.type)) {
                    statement(); 
                } else {
                    if (currentToken.type != END) {
                        addError(currentToken, "Expected a statement to follow after handling semicolon (or missing semicolon).");
                    }
                    break;
                }
            }
            if (currentToken.type == END) {
                consumeToken();
            } else {
                Token errorToken = (currentToken.type == PERIOD || currentToken.type == EOFS) ? currentToken : previousToken;
                addError(errorToken, "Expected 'END' keyword to close BEGIN...END statement.");
            }
            break;

        case IF:
            consumeToken();
            props = condition();
            if (currentToken.type == THEN) {
                consumeToken();
            } else {
                Error(previousToken, "Expected 'THEN' after condition in IF statement");
            }
            statement();
            if (currentToken.type == ELSE) {
                consumeToken();
                statement();
            }
            break;

        case WHILE:
            consumeToken();
            props = condition();
            if (currentToken.type == DO) {
                consumeToken();
            } else {
                Error(previousToken, "Expected 'DO' after condition in WHILE statement");
            }
            statement();
            break;

        case FOR:
            consumeToken();
            if (currentToken.type == IDENT) {
                identToken = currentToken;
                loc = Location(identToken.lexeme);
                if (loc == 0) {
                    Error(identToken, "Variable not declared for loop variable");
                    return;
                } else {
                    sym = &symbolTable[loc - 1];
                    if (sym->kind != KIND_VAR || sym->type != TYPE_INTEGER) {
                        Error(identToken, "Loop control variable must be an INTEGER variable");
                        return;
                    }
                }
                consumeToken();
            } else {
                Error(previousToken, "Expected identifier for loop variable after FOR");
            }
            if (currentToken.type == ASSIGN) {
                consumeToken();
            } else {
                Error(previousToken, "Expected ':=' after loop variable in FOR statement");
            }
            
            props = expression();
            if (props.type != TYPE_INTEGER && props.type != TYPE_ERROR) {
                Error(previousToken, "FOR loop start expression must be INTEGER type");
                return;
            }
            if (currentToken.type == TO) {
                consumeToken();
            } else {
                Error(previousToken, "Expected 'TO' after starting value in FOR statement");
            }
            props2 = expression();
            if (props2.type != TYPE_INTEGER && props2.type != TYPE_ERROR) {
                Error(previousToken, "FOR loop end expression must be INTEGER type.");
            }
            if (currentToken.type == DO) {
                consumeToken();
            } else {
                Error(previousToken, "Expected 'DO' after ending value in FOR statement");
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
        compileDeclareConstant();
        while (currentToken.type == COMMA) {
            consumeToken(); 
            compileDeclareConstant();
        }

        if (currentToken.type == SEMICOLON) {
            consumeToken();
        } else {
            addError(previousToken, "Expected ';' after CONST declarations");
        }
    } 

    if (currentToken.type == VAR) {
        consumeToken(); 
        compileDeclareVariable();
        while (currentToken.type == COMMA) {
            consumeToken(); 
            compileDeclareVariable();
        }
        if (currentToken.type == SEMICOLON) {
            consumeToken(); 
        } else {
            addError(previousToken, "Expected ';' after VAR declarations");
        }
    } 

    while (currentToken.type == PROCEDURE) {
        consumeToken(); 
        compileDeclareProcedure();
        
        if (currentToken.type == SEMICOLON) {
            consumeToken(); 
        } else {
            addError(previousToken, "Expected ';' after procedure block");
        }
    } 

    if (currentToken.type == BEGIN) { 
        consumeToken(); 
        if (isStartOfStatement(currentToken.type)) { 
            statement(); 
        } else if (currentToken.type != END) {
             addError(currentToken, "Expected a statement or END after BEGIN.");
        }
        while (currentToken.type == SEMICOLON || isStartOfStatement(currentToken.type)) {
            Token tokenForMissingSemicolonError = currentToken;
            if (currentToken.type == SEMICOLON) {
                consumeToken(); 
                if (!isStartOfStatement(currentToken.type)) {
                    if (currentToken.type == END) {
                    break;
                }
                    addError(currentToken, "Expected a statement or END after ';'.");
                    break;
                }
            } else {
                addError(tokenForMissingSemicolonError, "Missing ';' before this statement.");
            }

            if (isStartOfStatement(currentToken.type)) {
                statement(); 
            } else {
                if (currentToken.type != END) { 
                    addError(currentToken, "Expected a statement to follow."); 
                }
                break; 
            }
        }
        if (currentToken.type == END) {
            consumeToken(); 
        } else {
            Token errorToken = (currentToken.type == PERIOD || currentToken.type == EOFS) ? currentToken : previousToken;
            addError(errorToken, "Expected 'END' keyword to close the block.");
        }
    }
    else { 
    Error(previousToken, "Expected 'BEGIN' keyword to start the block body after declarations"); 
    }
} 

void program() {
    if (currentToken.type == PROGRAM) { 
        consumeToken(); 
    } else {
        Error(previousToken, "Program must start with 'PROGRAM' keyword");
    }

    if (currentToken.type == IDENT) { 
        consumeToken(); 
    } else {
        Error(previousToken, "Expected program name (identifier) after 'PROGRAM'");
    }

    if (currentToken.type == SEMICOLON) { 
        consumeToken(); 
    } else {
        addError(previousToken, "Expected ';' after program name. Continuing parse.");
    }
    currentLevel = 0;
    scope_stack_ptr++;
    if (scope_stack_ptr >= MAX_NESTING_DEPTH) {
        fprintf(stderr, "Critical Error: Maximum nesting depth exceeded.\n");
        exit(EXIT_FAILURE);
    }
    scope_stack[scope_stack_ptr] = symbolCount;
    block();

    if (currentToken.type == PERIOD) { 
        consumeToken();
    } else {
        Error(previousToken, "Program must end with a '.'");
    }

    if (currentToken.type != EOFS) {
        Error(previousToken, "Unexpected tokens after the final '.'");
    }
}

void EnterInputOutputStatement(){
    Enter("READLN", KIND_PROC, TYPE_NONE, 0, 0);
    if (symbolCount > 0) {
        Symbol* readln_sym = &symbolTable[symbolCount - 1];
        readln_sym->numParams = 1;
        readln_sym->formalParamTypes[0] = TYPE_INTEGER;
    }
    Enter("WRITELN", KIND_PROC, TYPE_NONE, 0, 0);
    if (symbolCount > 0) {
        Symbol* writeln_sym = &symbolTable[symbolCount - 1];
        writeln_sym->numParams = 1;
        writeln_sym->formalParamTypes[0] = TYPE_INTEGER;
    }
    Enter("READ", KIND_PROC, TYPE_NONE, 0, 0);
    if (symbolCount > 0) {
        Symbol* read_sym = &symbolTable[symbolCount - 1];
        read_sym->numParams = 1;
        read_sym->formalParamTypes[0] = TYPE_INTEGER;
    }
    Enter("WRITE", KIND_PROC, TYPE_NONE, 0, 0);
    if (symbolCount > 0) {
        Symbol* write_sym = &symbolTable[symbolCount - 1];
        write_sym->numParams = 1;
        write_sym->formalParamTypes[0] = TYPE_INTEGER;
    }
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

    EnterInputOutputStatement();
    consumeToken();

    program();

    if (compilationErrorOccurred) {
        printf("\nCompilation failed due to errors listed above.\n");
    } else {
        printf("\nSemantic analysis successful! (No syntax/semantic errors detected by this phase)\n");
        printf("\nSymbol Table:\n");
        printf("%-15s %-10s %-10s %-10s %-10s %-10s %-10s %-10s %-10s \n",
               "Name", "Kind", "Type", "ElemType", "Value", "Size", "Level" , "Address", "NumParams");
        printf("------------------------------------------------------------------------------------------\n"); 
        for (int i = 0; i < symbolCount; i++) {
            const char *kindStr, *typeStr, *elemTypeStr;
            switch (symbolTable[i].kind) {
                case KIND_CONST: kindStr = "CONST"; break;
                case KIND_VAR: kindStr = "VAR"; break;
                case KIND_PROC: kindStr = "PROC"; break;
                default: kindStr = "UNKNOWN";
            }
            typeStr = datatype_to_string(symbolTable[i].type);
            elemTypeStr = datatype_to_string(symbolTable[i].elementType);

            printf("%-15s %-10s %-10s %-10s %-10d %-10d %-10d %-10d %-10d ",
                   symbolTable[i].name,
                   kindStr,
                   typeStr,
                   elemTypeStr,
                   symbolTable[i].value,
                   symbolTable[i].size,
                   symbolTable[i].level,
                   symbolTable[i].address,
                   symbolTable[i].numParams
            );
            if (symbolTable[i].numParams > 0) {
                printf("[");
                for (int j = 0; j < symbolTable[i].numParams; j++) {
                    printf("%s", datatype_to_string(symbolTable[i].formalParamTypes[j]));
                    if (j < symbolTable[i].numParams - 1) printf(", ");
                }
                printf("]");
            }
            printf("\n");
        }
    }
    fclose(inputFile);
    return compilationErrorOccurred ? EXIT_FAILURE : EXIT_SUCCESS;
}