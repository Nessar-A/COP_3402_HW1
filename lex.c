/*
Assignment:
lex - Lexical Analyzer for PL/0

Author: Nessar Amiri, Marcelo Pacheco

Language: C(only)

To Compile:
    gcc -O2 -std=c11 -o lex lex.c
    
To Execute (on Eustis):
    ./lex <input file>
    
where:
    <input file> is the path to the PL/0 source program
    
Notes:
    - Implement a lexical analyser for the PL/0 language
    - The program must detect errors such as
        - numbers longer than five digits
        - identifiers longer than eleven characters
        - invalid characters
    - The output format must exactly match the specification
    - Tested on Eustis.
    
Class: COP 3402 - System Software - Fall 2025

Instructor: Dr. Jie Lin

Due Date: Friday, October 3, 2025 at 11:59 PM ET
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ARRAY_SIZE 500       // maximum size of arrays to store lexemes and tokens
#define MAX_IDENT 11             // maximum allowed identifier length
#define MAX_NUM 5                // maximum allowed number length

// Enumeration of all possible token types for PL/0 language
typedef enum {
    // Token Symbol    // Token Name             // Token Number

    skipsym = 1,       // Skips or errors
    identsym,          // Identifier             2
    numbersym,         // Number                 3
    plussym,           // +                      4
    minussym,          // -                      5
    multsym,           // *                      6
    slashsym,          // /                      7
    eqsym,             // =                      8
    neqsym,            // <>                     9
    lessym,            // <                      10
    leqsym,            // <=                     11
    gtrsym,            // >                      12
    geqsym,            // >=                     13
    lparentsym,        // (                      14
    rparentsym,        // )                      15
    commasym,          // ,                      16
    semicolonsym,      // ;                      17
    periodsym,         // .                      18
    becomessym,        // :=                     19
    beginsym,          // begin                  20
    endsym,            // end                    21
    ifsym,             // if                     22
    fisym,             // fi                     23
    thensym,           // then                   24
    whilesym,          // while                  25
    dosym,             // do                     26
    callsym,           // call                   27
    constsym,          // const                  28
    varsym,            // var                     29
    procsym,           // procedure              30
    writesym,          // write                  31
    readsym,           // read                   32
    elsesym,           // else                   33
    evensym,           // even                   34
} TokenType;

// Structure to represent each token
typedef struct {
    char lexeme[MAX_ARRAY_SIZE];    // stores the actual string of the token
    TokenType type;                 // type of token
    char errorMessage[100];         // stores an error message if token is invalid
} Token;

// Global array to hold all tokens
Token tokenArray[MAX_ARRAY_SIZE];
int tokenLocation = 0;              // keeps track of the next empty spot in the array

// ----------------------------------------------------
// Checks if a word is a reserved keyword or an identifier
// ----------------------------------------------------
TokenType reservedWords(char *word) {
    if(!strcmp(word, "begin"))
        return beginsym;
    if(!strcmp(word, "end"))
        return endsym;
    if(!strcmp(word, "if")) 
        return ifsym;
    if(!strcmp(word, "fi")) 
        return fisym;
    if(!strcmp(word, "then")) 
        return thensym;
    if(!strcmp(word, "while")) 
        return whilesym;
    if(!strcmp(word, "do")) 
        return dosym;
    if(!strcmp(word, "call"))  
        return callsym;
    if(!strcmp(word, "const")) 
        return constsym;
    if(!strcmp(word, "var")) 
        return varsym;
    if(!strcmp(word, "procedure")) 
        return procsym;
    if(!strcmp(word, "write")) 
        return writesym;
    if(!strcmp(word, "read")) 
        return readsym;
    if(!strcmp(word, "else")) 
        return elsesym;
    if(!strcmp(word, "even")) 
        return evensym;

    return identsym;    // if not a reserved word, it is an identifier
}

// ----------------------------------------------------
// Adds a token into the global token array
// ----------------------------------------------------
void addToArray(char *lex, TokenType type, char *error) {
    strcpy(tokenArray[tokenLocation].lexeme, lex);      // store full lexeme
    tokenArray[tokenLocation].type = type;              // store token type

    if (error)
        strcpy(tokenArray[tokenLocation].errorMessage, error);  // store error message if present
    else
        tokenArray[tokenLocation].errorMessage[0] = '\0';       // no error

    tokenLocation++;                                     // move to next position
}

// ----------------------------------------------------
// Skips over comments inside /* ... */
// ----------------------------------------------------
void skip(FILE *f) {
    int ch, prevChar = 0;
    while((ch = fgetc(f)) != EOF) {              // read until end of file or comment closes
        if (prevChar == '*' && ch == '/')        // detect end of comment
            return;                              // finished skipping comment
        prevChar = ch;                           // update previous character
    }
}

// ----------------------------------------------------
// Scans identifiers or reserved words starting with a letter
// ----------------------------------------------------
void scanWord(FILE *f, int firstChar) {
    char temp[MAX_ARRAY_SIZE];                   // temporary buffer
    int length = 0;

    temp[length++] = firstChar;                   // store first character

    int ch;
    while ((ch = fgetc(f)) != EOF && isalnum(ch))  // continue while alphanumeric
        temp[length++] = ch;
    
    temp[length] = '\0';                           // null-terminate string

    if (ch != EOF)
        ungetc(ch, f);                             // push back the non-alphanumeric character for next scan

    if (length > MAX_IDENT)                        // check for identifier length
        addToArray(temp, skipsym, "Identifier too long");
    else
        addToArray(temp, reservedWords(temp), NULL);
}

// ----------------------------------------------------
// Scans numbers starting with a digit
// ----------------------------------------------------
void scanNumber(FILE *f, int firstNum) {
    char temp[MAX_ARRAY_SIZE];                    // temporary buffer
    int length = 0;

    temp[length++] = firstNum;                     // store first digit

    int ch;
    while ((ch = fgetc(f)) != EOF && isdigit(ch))   // continue while digit
        temp[length++] = ch;
    
    temp[length] = '\0';                            // null-terminate string

    if (ch != EOF)
        ungetc(ch, f);                              // push back non-digit character for next scan

    if (length > MAX_NUM)                           // check for number length
        addToArray(temp, skipsym, "Number too long");
    else
        addToArray(temp, numbersym, NULL);
}

// ----------------------------------------------------
// Scans operators that may have two characters (:=, <=, >=, <>)
// ----------------------------------------------------
void scanTwoOperator(FILE *f, int firstChar) {
    int nextChar = fgetc(f);                             // look ahead to next character
    char operator[3] = { (char)firstChar, (char)nextChar, '\0'};
    TokenType type;

    if (firstChar == '<' && nextChar == '>')
        type = neqsym;                                    // <>
    else if (firstChar == '<' && nextChar == '=')
        type = leqsym;                                    // <=
    else if (firstChar == '>' && nextChar == '=')
        type = geqsym;                                    // >=
    else if (firstChar == ':' && nextChar == '=')
        type = becomessym;                                // :=
    else {
        if (nextChar != EOF)
            ungetc(nextChar, f);                          // not a two-character operator, push back

        operator[1] = '\0';                                // single-character operator
        if (firstChar == '<')
            type = lessym;                                 // <
        else if (firstChar == '>')
            type = gtrsym;                                 // >
        else if (firstChar == ':') {
            type = skipsym;                                // invalid ':' without '='
            addToArray(operator, type, "Invalid symbol");
            return;
        }
    }
    addToArray(operator, type, NULL);
}

// ----------------------------------------------------
// Scans single-character operators or invalid symbols
// ----------------------------------------------------
void scanOperator(int ch) {
    char operator[2] = {(char) ch, '\0'};
    if (ch == '+')
        addToArray(operator, plussym, NULL);
    else if (ch == '-')
        addToArray(operator, minussym, NULL);
    else if (ch == '*')
        addToArray(operator, multsym, NULL);
    else if (ch == '/')
        addToArray(operator, slashsym, NULL);
    else if (ch == '=')
        addToArray(operator, eqsym, NULL);
    else if (ch == '(')
        addToArray(operator, lparentsym, NULL);
    else if (ch == ')')
        addToArray(operator, rparentsym, NULL);
    else if (ch == ',')
        addToArray(operator, commasym, NULL);
    else if (ch == ';')
        addToArray(operator, semicolonsym, NULL);
    else if (ch == '.')
        addToArray(operator, periodsym, NULL);
    else
        addToArray(operator, skipsym, "Invalid symbol");  // invalid character
}

// ----------------------------------------------------
// Main scanner function to process the file
// ----------------------------------------------------
void scanFile(FILE *f) {
    int ch;

    while ((ch = fgetc(f)) != EOF) {                   // read each character

        if (isspace(ch))                               // ignore spaces, tabs, and newlines
            continue;

        if (isalpha(ch))                               // handle identifiers and reserved words
            scanWord(f, ch);

        else if (isdigit(ch))                          // handle numbers
            scanNumber(f, ch);

        else if (ch == '/') {                          // could be comment or division
            int nextChar = fgetc(f);
            if (nextChar == '*')
                skip(f);                               // skip comment
            else {
                if (nextChar != EOF)
                    ungetc(nextChar, f);               // not a comment, push back character
                addToArray("/", slashsym, NULL);
            }
        }

        else if (ch == '<' || ch == '>' || ch == ':')   // possible two-character operators
            scanTwoOperator(f, ch);

        else                                           // all other single-character operators or errors
            scanOperator(ch);
    }
}

// ----------------------------------------------------
// Writes the token list to tokens.txt (same format as before)
// ----------------------------------------------------
void printTokens(void) {
    FILE *out = fopen("tokens.txt", "w");
    if (!out) {
        perror("Unable to open tokens.txt for write");
        return;
    }

    for (int i = 0; i < tokenLocation; i++) {
        if (tokenArray[i].errorMessage[0]) {
            // skip/error token
            fprintf(out, "1\n");
        } else if (tokenArray[i].type == identsym || tokenArray[i].type == numbersym) {
            // type + lexeme
            fprintf(out, "%d %s\n", tokenArray[i].type, tokenArray[i].lexeme);
        } else {
            // type only
            fprintf(out, "%d\n", tokenArray[i].type);
        }
    }
    fprintf(out, "\n");
    fclose(out);
}

// ----------------------------------------------------
// Main driver of the program
// ----------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        return 1;                         // exit with error
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("Unable to open input file");
        return 1;
    }

    scanFile(f);
    fclose(f);

    printTokens();

    return 0;
}

