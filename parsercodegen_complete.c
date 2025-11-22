/*
Assignment:
HW4 - Complete Parser and Code Generator for PL/0
      (with Procedures, Call, and Else)
Author(s): Nessar Amiri, Marcelo Pacheco

Language: C (only)

To Compile:
  Scanner:
    gcc -O2 -std=c11 -o lex lex.c
  Parser/Code Generator:
    gcc -O2 -std=c11 -o parsercodegen_complete parsercodegen_complete.c
  Virtual Machine:
    gcc -O2 -std=c11 -o vm vm.c

To Execute (on Eustis):
  ./lex <input_file.txt>
  ./parsercodegen_complete
  ./vm elf.txt

where:
<input_file.txt> is the path to the PL/0 source program

Notes:
  - lex.c accepts ONE command-line argument (input PL/0 source file)
  - parsercodegen_complete.c accepts NO command-line arguments
  - Input filename is hard-coded in parsercodegen_complete.c
  - Implements recursive-descent parser for extended PL/0 grammar
  - Supports procedures, call statements, and if-then-else
  - Generates PM/0 assembly code (see Appendix A for ISA)
  - VM must support EVEN instruction (OPR 0 11)
  - All development and testing performed on Eustis

Class: COP3402 - System Software - Fall 2025

Instructor: Dr. Jie Lin

Due Date: Friday, November 21, 2025 at 11:59 PM ET
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_FILE "tokens.txt"
#define ELF_FILE "elf.txt"
#define MAX_TOKENS 500
#define MAX_CODE 10000 //Random high number (so we don't accidentally have overflow)
#define MAX_SYMBOL_TABLE_SIZE 500

// Enumeration of all possible token types for PL/0 language
enum {
    // Token Symbol // Token Name // Token Number
    skipsym = 1, // Skips or errors
    identsym,    // Identifier 2
    numbersym,   // Number 3
    plussym,     // + 4
    minussym,    // - 5
    multsym,     // * 6
    slashsym,    // / 7
    eqsym,       // = 8
    neqsym,      // <> 9
    lessym,      // < 10
    leqsym,      // <= 11
    gtrsym,      // > 12
    geqsym,      // >= 13
    lparentsym,  // ( 14
    rparentsym,  // ) 15
    commasym,    // , 16
    semicolonsym,// ; 17
    periodsym,   // . 18
    becomessym,  // := 19
    beginsym,    // begin 20
    endsym,      // end 21
    ifsym,       // if 22
    fisym,       // fi 23
    thensym,     // then 24
    whilesym,    // while 25
    dosym,       // do 26
    callsym,     // call 27
    constsym,    // const 28
    varsym,      // var 29
    procsym,     // procedure 30
    writesym,    // write 31
    readsym,     // read 32
    elsesym,     // else 33
    evensym,     // even 34
};

typedef struct {
    int OP;
    int L;
    int M;
} instruction;

enum {
    OP_LIT = 1,
    OP_OPR,
    OP_LOD,
    OP_STO,
    OP_CAL,
    OP_INC,
    OP_JMP,
    OP_JPC,
    OP_SYS
};

enum {
    OPR_RET,
    OPR_ADD,
    OPR_SUB,
    OPR_MUL,
    OPR_DIV,
    OPR_EQL,
    OPR_NEQ,
    OPR_LSS,
    OPR_LEQ,
    OPR_GTR,
    OPR_GEQ,
    OPR_EVEN
};

typedef struct {
    int type;
    char lexeme[MAX_TOKENS];
} Token;

static Token tokens[MAX_TOKENS];
static int totalToken = 0, tokenIndex = 0;

typedef struct {
    int kind;      // const = 1, var = 2, proc = 3
    char name[12]; // name up to 11 chars
    int val;       // number (ASCII value)
    int level;     // L level
    int addr;      // M address
    int mark;      // to indicate unavailable or deleted
} symbol;

static symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
static int totalSymbols = 0;

static instruction code[MAX_CODE]; //Code buffer
static int codeIndex = 0;

static int currentLevel = 0;            //This will be a lexical level tracker

static FILE* fp = NULL;

static void printError(int errorCode, FILE* output) {
    // prints errors based on codes
    // to terminal & file
    printf("Error: ");
    fprintf(output, "Error: ");
    if (errorCode == 0) {
        printf("Scanning error detected by lexer (skipsym present)\n");
        fprintf(output, "Scanning error detected by lexer (skipsym present)\n");
    } else if (errorCode == 1) {
        printf("program must end with period\n");
        fprintf(output, "program must end with period\n");
    } else if (errorCode == 2) {
        printf("const, var, read, procedure, and call keywords must be followed by identifier\n");
        fprintf(output, "const, var, read, procedure, and call keywords must be followed by identifier\n");
    } else if (errorCode == 3) {
        printf("symbol name has already been declared\n");
        fprintf(output, "symbol name has already been declared\n");
    } else if (errorCode == 4) {
        printf("constants must be assigned with =\n");
        fprintf(output, "constants must be assigned with =\n");
    } else if (errorCode == 5) {
        printf("constants must be assigned an integer value\n");
        fprintf(output, "constants must be assigned an integer value\n");
    } else if (errorCode == 6) {
        printf("constant and variable declarations must be followed by a semicolon\n");
        fprintf(output, "constant and variable declarations must be followed by a semicolon\n");
    } else if (errorCode == 7) {
        printf("undeclared identifier\n");
        fprintf(output, "undeclared identifier\n");
    } else if (errorCode == 8) {
        printf("only variable values may be altered\n");
        fprintf(output, "only variable values may be altered\n");
    } else if (errorCode == 9) {
        printf("assignment statements must use :=\n");
        fprintf(output, "assignment statements must use :=\n");
    } else if (errorCode == 10) {
        printf("begin must be followed by end\n");
        fprintf(output, "begin must be followed by end\n");
    } else if (errorCode == 11) {
        printf("if must be followed by then\n");
        fprintf(output, "if must be followed by then\n");
    } else if (errorCode == 12) {
        printf("else must be followed by fi\n");
        fprintf(output, "else must be followed by fi\n");
    } else if (errorCode == 13) {
        printf("if statement must include else clause\n");
        fprintf(output, "if statement must include else clause\n");
    } if (errorCode == 14) {
        printf("while must be followed by do\n");
        fprintf(output, "while must be followed by do\n");
    } else if (errorCode == 15) {
        printf("condition must contain comparison operator\n");
        fprintf(output, "condition must contain comparison operator\n");
    } else if (errorCode == 16) {
        printf("right parenthesis must follow left parenthesis\n");
        fprintf(output, "right parenthesis must follow left parenthesis\n");
    } else if (errorCode == 17) {
        printf("arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
        fprintf(output, "arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
    } else if (errorCode == 18) {
        printf("call statement may only target procedures\n");
        fprintf(output, "call statement may only target procedures\n");
    } else if (errorCode == 19) {
        printf("procedure declaration must be followed by a semicolon\n");
        fprintf(output, "procedure declaration must be followed by a semicolon\n");
    }
}

static void exitAndPrint(int errorCode) {
    // exits function and prints error
    if (!fp)
        fp = fopen(ELF_FILE, "w"); // opens file
    printError(errorCode, fp);       // prints error
    if (fp) {
        fclose(fp); // closes file
        fp = NULL;
    }
    exit(1);
}

static void emit(int OP, int L, int M) {
    // assigns OP, L, and M to code
    code[codeIndex].OP = OP;
    code[codeIndex].L = L;
    code[codeIndex].M = M;
    codeIndex++;
}

static int peekType(void) {
    // compares tokenIndex to totalToken
    if (tokenIndex < totalToken) {
        return tokens[tokenIndex].type;
    } else {
        return 0;
    }
}

static Token getToken(void) {
    // consumes and returns the next token
    Token t;
    if (tokenIndex < totalToken) {
        t = tokens[tokenIndex];
        tokenIndex++;
    } else {
        t.type = 0;
        t.lexeme[0] = '\0';
    }
    return t;
}

static int accept(int type) {
    // checks if token matches type
    if (peekType() == type) {
        if (tokenIndex < totalToken)
            tokenIndex++;
        return 1;
    }
    return 0;
}

static inline int targetAddr(int instrIndex) {
    return instrIndex * 3; // word address
}

static int SYMBOLTABLECHECK(const char *name) {
    int i = totalSymbols - 1; // index
    while (i >= 0) {
        if (symbol_table[i].mark == 0 && strcmp(symbol_table[i].name, name) == 0) return i; // if strings are equal
        i = i - 1;
    }
    return -1; // not equal
}

static void markOutOfScope(int currentLevel) {
    for(int i = totalSymbols - 1; i >= 0; i--)
        if(symbol_table[i].level > currentLevel)
            symbol_table[i].mark = 1;
}


static void addConst(const char *name, int value) {
    // adds constant
    symbol s; // symbol
    memset(&s, 0, sizeof(s));
    s.kind = 1;
    strncpy(s.name, name, sizeof(s.name));
    // assigns values to s
    s.name[sizeof(s.name) - 1] = '\0';
    s.val = value;
    s.level = currentLevel;
    s.addr = 0;
    s.mark = 0;
    symbol_table[totalSymbols++] = s;
}

static void addVar(const char *name, int addr) {
    // adds variable
    symbol s;
    memset(&s, 0, sizeof(s));
    s.kind = 2;
    strncpy(s.name, name, sizeof(s.name)); s.name[sizeof(s.name) - 1] = '\0';
    // assigns values to s
    s.val = 0;
    s.level = currentLevel;
    s.addr = addr;
    s.mark = 0;
    symbol_table[totalSymbols++] = s;
}

static void addProc(const char *name, int addr) {
    // adds procedures
    symbol s;
    memset(&s, 0, sizeof(s));
    s.kind = 3;
    strncpy(s.name, name, sizeof(s.name));
    s.name[sizeof(s.name) - 1] = '\0';
    // assigns values to s
    s.val = 0;
    s.level = currentLevel;
    s.addr = addr;
    s.mark = 0;
    symbol_table[totalSymbols++] = s;
}

// parser function prototypes
static void PROGRAM();
static void BLOCK();
static void CONST_DECLARATION();
static int VAR_DECLARATION();
static void PROCEDURE_DECLARATIONS();
static void STATEMENT();
static void CONDITION();
static void EXPRESSION();
static void TERM();
static void FACTOR();

static void PROGRAM() {
    BLOCK(); // fires block

    if (!accept(periodsym))
        exitAndPrint(1); // exits program
    emit(OP_SYS, 0, 3);

    currentLevel--;
    markOutOfScope(currentLevel);   // we do this because we are done with the outermost scope
}

static void BLOCK() {
    int skipIndex = codeIndex;
    emit(OP_JMP, 0, 0);     // placeholder

    CONST_DECLARATION();                // fires const_declaration
    int numVars = VAR_DECLARATION();    // gets variables
    PROCEDURE_DECLARATIONS();           // handles procedures before STATEMENT
    
    code[skipIndex].M = targetAddr(codeIndex);
    
    emit(OP_INC, 0, 3 + numVars);
    STATEMENT();                        // statement
}

static void CONST_DECLARATION() {
    if (accept(constsym)) {
        do {
            Token token = getToken();
            if (token.type != identsym)
                exitAndPrint(2); // exits program and prints error 2
            if (SYMBOLTABLECHECK(token.lexeme) != -1)
                exitAndPrint(3); // exits program and prints error 3
            if (!accept(eqsym))
                exitAndPrint(4); // exits program and prints error 4
            Token numToken = getToken(); // gets token
            if (numToken.type != numbersym)
                exitAndPrint(5); // exits program and prints error 5
            addConst(token.lexeme, atoi(numToken.lexeme));
        } while (accept(commasym));
        if (!accept(semicolonsym))
            exitAndPrint(6); // exits program and prints error 6
    }
}

static int VAR_DECLARATION() {
    int numVars = 0;
    if (accept(varsym)) {
        do {
            numVars++;
            Token token = getToken();
            if (token.type != identsym)
                exitAndPrint(2); // exits program and prints error 2
            if (SYMBOLTABLECHECK(token.lexeme) != -1)
                exitAndPrint(3); // exits program and prints error 3
            addVar(token.lexeme, 3 + (numVars - 1));
        } while (accept(commasym));
        if (!accept(semicolonsym))
            exitAndPrint(6); // exits program and prints error 6
    }
    return numVars;
}

static void PROCEDURE_DECLARATIONS() {
    while (accept(procsym)) {
        Token token = getToken();
        if (token.type != identsym)
            exitAndPrint(2);    // exits program and prints error 2
        if (SYMBOLTABLECHECK(token.lexeme) != -1)
            exitAndPrint(3);     // exits program and prints error 3
        
        int entryAddr = codeIndex;
        addProc(token.lexeme, targetAddr(entryAddr));
        
        if(!accept(semicolonsym))
            exitAndPrint(19);    // exits program and prints error 19

        currentLevel++;
        BLOCK();
        currentLevel--;

        markOutOfScope(currentLevel);

        if(!accept(semicolonsym))
            exitAndPrint(19);    // exits program and prints error 19

        emit(OP_OPR, 0, OPR_RET);

    }
}
static void STATEMENT() {
    int temp = peekType();
    if (temp == identsym) {
        Token token = getToken();
        int symIdx = SYMBOLTABLECHECK(token.lexeme);
        if (symIdx == -1)
            exitAndPrint(7); // exits program and prints error 7
        if (symbol_table[symIdx].kind != 2)
            exitAndPrint(8); // exits program and prints error 8
        if (!accept(becomessym))
            exitAndPrint(9); // exits program and prints error 9
        EXPRESSION();

        int levelDiff = currentLevel - symbol_table[symIdx].level;
        emit(OP_STO, levelDiff, symbol_table[symIdx].addr);
        return;
    }

    if (accept(callsym)) {
        Token token = getToken();
        if (token.type != identsym)
            exitAndPrint(2);
        
        int symIdx = SYMBOLTABLECHECK(token.lexeme);
        if (symIdx == -1)
            exitAndPrint(7);
        if (symbol_table[symIdx].kind != 3)
            exitAndPrint(18);

        int levelDiff = currentLevel - symbol_table[symIdx].level;
        emit(OP_CAL, levelDiff, symbol_table[symIdx].addr);
        return;
    }

    if (accept(beginsym)) {
        do {
            STATEMENT();
        } while (accept(semicolonsym));
        if (!accept(endsym))
            exitAndPrint(10); // exits program and prints error 10
        return;
    }
    if (accept(ifsym)) {
        CONDITION();
        if (!accept(thensym)) 
            exitAndPrint(11);

        int jpcThenIdx = codeIndex;
        emit(OP_JPC, 0, 0);

        STATEMENT();

        if (!accept(elsesym)) 
            exitAndPrint(13);

        int jmpAfterIdx = codeIndex;
        emit(OP_JMP, 0, 0);

        code[jpcThenIdx].M = targetAddr(codeIndex);

        STATEMENT();

        if (!accept(fisym)) 
            exitAndPrint(12);

        code[jmpAfterIdx].M = targetAddr(codeIndex);
        return;
    }
    if (accept(whilesym)) {
        int loopIdx = codeIndex;
        CONDITION();
        if (!accept(dosym))
            exitAndPrint(14); // exits program and prints error 14
        int jpcIdx = codeIndex;
        emit(OP_JPC, 0, 0);
        STATEMENT();
        emit(OP_JMP, 0, targetAddr(loopIdx));
        code[jpcIdx].M = targetAddr(codeIndex);
        return;
    }
    if (accept(readsym)) {
        Token token = getToken();
        if (token.type != identsym)
            exitAndPrint(2); // exits program and prints error 2
        int symIdx = SYMBOLTABLECHECK(token.lexeme);
        if (symIdx == -1)
            exitAndPrint(7); // exits program and prints error 7
        if (symbol_table[symIdx].kind != 2)
            exitAndPrint(8); // exits program and prints error 8

        int levelDiff = currentLevel - symbol_table[symIdx].level;
        emit(OP_SYS, 0, 2);
        emit(OP_STO, levelDiff, symbol_table[symIdx].addr);
        return;
    }
    if (accept(writesym)) {
        EXPRESSION();
        emit(OP_SYS, 0, 1);
        return;
    }
}

static void CONDITION() {
    if (accept(evensym)) {
        EXPRESSION();
        emit(OP_OPR, 0, OPR_EVEN);
        return;
    }
    EXPRESSION();
    if (accept(eqsym)) {
        EXPRESSION();
        emit(OP_OPR, 0, OPR_EQL);
        return;
    } else if (accept(neqsym)) {
        EXPRESSION();
        emit(OP_OPR, 0, OPR_NEQ);
        return;
    } else if (accept(lessym)) {
        EXPRESSION();
        emit(OP_OPR, 0, OPR_LSS);
        return;
    } else if (accept(leqsym)) {
        EXPRESSION();
        emit(OP_OPR, 0, OPR_LEQ);
        return;
    } else if (accept(gtrsym)) {
        EXPRESSION();
        emit(OP_OPR, 0, OPR_GTR);
        return;
    } else if (accept(geqsym)) {
        EXPRESSION();
        emit(OP_OPR, 0, OPR_GEQ);
        return;
    }
    exitAndPrint(15); // exits program and prints error 15
}

static void EXPRESSION() {
    TERM();
    while (1) {
        if (accept(plussym)) {
            TERM();
            emit(OP_OPR, 0, OPR_ADD);
        } else if (accept(minussym)) {
            TERM();
            emit(OP_OPR, 0, OPR_SUB);
        } else {
            break;
        }
    }
}

static void TERM() {
    FACTOR();
    while (peekType() == multsym || peekType() == slashsym) {
        if (accept(multsym)) {
            FACTOR();
            emit(OP_OPR, 0, OPR_MUL);
        } else if (accept(slashsym)) {
            FACTOR();
            emit(OP_OPR, 0, OPR_DIV);
        }
    }
}

static void FACTOR() {
    int temp = peekType();
    if (temp == identsym) {
        Token token = getToken();
        int symIdx = SYMBOLTABLECHECK(token.lexeme);
        if (symIdx == -1)
            exitAndPrint(7); // exits program and prints error 7

        if (symbol_table[symIdx].kind == 1) {
            emit(OP_LIT, 0, symbol_table[symIdx].val);
        } else if (symbol_table[symIdx].kind == 2) {
            int levelDiff = currentLevel - symbol_table[symIdx].level;
            emit(OP_LOD, levelDiff, symbol_table[symIdx].addr);
        } else {
            exitAndPrint(17); // exits program and prints error 17
        }
        return;
    }
    if (temp == numbersym) {
        Token numToken = getToken();
        emit(OP_LIT, 0, atoi(numToken.lexeme));
        return;
    }
    if (accept(lparentsym)) {
        EXPRESSION();
        if (!accept(rparentsym))
            exitAndPrint(16); // exits program and prints error 16
        return;
    }
    exitAndPrint(17);
}

static const char* opMnemonic(int op) {
    // mnemonic for op
    switch (op) {
        case OP_LIT: return "LIT";
        case OP_OPR: return "OPR";
        case OP_LOD: return "LOD";
        case OP_STO: return "STO";
        case OP_CAL: return "CAL";
        case OP_INC: return "INC";
        case OP_JMP: return "JMP";
        case OP_JPC: return "JPC";
        case OP_SYS: return "SYS";
        default:     return "?";
    }
}

static void printSymbolTable(void) {
    printf("\nSymbol Table: \n\n");
    printf("%-7s | %-12s | %-8s | %-7s | %-7s | %-5s\n",
           "Kind", "Name", "Val", "Level", "Addr", "Mark");
    printf("------------------------------------------------------------\n");
    for (int i = 0; i < totalSymbols; i++) {
        printf("%-7d | %-12s | %-8d | %-7d | %-7d | %-5d\n",
               symbol_table[i].kind,
               symbol_table[i].name,
               symbol_table[i].val,
               symbol_table[i].level,
               symbol_table[i].addr,
               symbol_table[i].mark);
    }
}

static void printAssembly(void) {
    // prints the assembly
    printf("Assembly Code\n\n");
    printf("%-5s %-4s %-3s %-4s\n", "Line", "OP", "L", "M");
    printf("------------------------------------------------------------\n");
    for (int i = 0; i < codeIndex; i++) {
        // runs code to print assembly
        const char *op = opMnemonic(code[i].OP);
        printf("%-5d %-4s %-3d %-4d\n", i, op, code[i].L, code[i].M);
    }
}

static void printTerminal(void) {
    printAssembly();
    printSymbolTable();
}

static void loadTokensOrExit(void) {
    FILE *f = fopen(TOKEN_FILE, "r");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", TOKEN_FILE); // unable to open input
        exit(1);
    }
    totalToken = 0;
    while (!feof(f) && totalToken < MAX_TOKENS) {
        int tokenType; // tokenType
        if (fscanf(f, "%d", &tokenType) != 1) break; // breaks loop
        tokens[totalToken].type = tokenType; // assigns type to token
        tokens[totalToken].lexeme[0] = '\0'; // assigns lexeme to token
        if (tokenType == identsym || tokenType == numbersym) {
            if (fscanf(f, "%499s", tokens[totalToken].lexeme) != 1) {
                // ends program and prints error
                fclose(f);
                exitAndPrint(17);
            }
        }
        totalToken++;
    }
    fclose(f);
    for (int i = 0; i < totalToken; i++) {
        if (tokens[i].type == skipsym) exitAndPrint(0);
    }
}

static void writeElf(void) {
    FILE *out = fopen(ELF_FILE, "w"); // opens elf.txt
    if (!out) {
        fprintf(stderr, "Cannot open %s for write\n", ELF_FILE); // unable to open elf.txt
        exit(1);
    }
    for (int i = 0; i < codeIndex; i++) { // prints to output
        fprintf(out, "%d %d %d\n", code[i].OP, code[i].L, code[i].M);
    }
    fclose(out);
}

int main(void) {
    loadTokensOrExit();
    PROGRAM();
    printTerminal();
    writeElf();
    return 0;
}
