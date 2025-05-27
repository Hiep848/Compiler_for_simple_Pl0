# Compiler for Simple Pl0

## First Phase: Lexical Analysis

The first phase of the compiler is **Lexical Analysis**. This phase scans the source code and converts it into a sequence of tokens, which are the basic building blocks of the language.

- **Input:** Source code (text)
- **Output:** Tokens (identifiers, keywords, symbols, etc.)
- **Tasks:**
    - Removing whitespace and comments
    - Recognizing keywords, identifiers, literals, and operators
    - Reporting lexical errors

## Second Phase: Syntax Analysis

The second phase is **Syntax Analysis** (Parsing). It takes the tokens from the lexical analyzer and arranges them into a parse tree according to the grammar of the language.

- **Input:** Tokens
- **Output:** Correct grammar structure confirmation or error
- **Tasks:**
    - Checking for syntax errors
    - Building the hierarchical structure of the program

## Third Phase: Semantic Analysis

The third phase is **Semantic Analysis**. This phase checks the parse tree for semantic errors and gathers type information.

- **Input:** Tokens
- **Output:** Symbol table & errors
- **Tasks:**
    - Type checking
    - Scope resolution
    - Detecting semantic errors (e.g., undeclared variables)

## Fourth Phase: Code Generation

The fourth phase is **Code Generation**. Here, the intermediate representation is translated into target machine code or assembly instructions.

- **Input:** Intermediate code (e.g., AST or three-address code)
- **Output:** Target machine code or assembly
- **Tasks:**
    - Instruction selection and mapping
    - Register allocation and management
    - Handling variables, constants, and control flow
    - Emitting code for expressions, statements, and procedures
    - Reporting code generation errors

This phase ensures the compiled program can be executed efficiently on the target platform.