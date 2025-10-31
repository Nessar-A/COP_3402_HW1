/*
Assignment:
HW3 - Parser and Code Generator for PL/0

Author(s): <Full Name 1>, <Full Name 2>

Language: C (only)

To Compile:
    Scanner:
        gcc -O2 -std=c11 -o lex lex.c
    Parser/Code Generator:
        gcc -O2 -std=c11 -o parsercodegen parsercodegen.c

    To Execute (on Eustis):
        ./lex <input_file.txt>
        ./parsercodegen

where:
    <input_file.txt> is the path to the PL/0 source program

Notes:
    - lex.c accepts ONE command-line argument (input PL/0 source file)
    - parsercodegen.c accepts NO command-line arguments
    - Input filename is hard-coded in parsercodegen.c
    - Implements recursive-descent parser for PL/0 grammar
    - Generates PM/0 assembly code (see Appendix A for ISA)
    - All development and testing performed on Eustis

Class: COP3402 - System Software - Fall 2025

Instructor: Dr. Jie Lin

Due Date: Friday, October 31, 2025 at 11:59 PM ET
*/

/*
    TO-DO LIST:
    1. Once program is finished, change all printf statements to fprintf, to print to elf.txt
*/ 

#include <stdio.h>

void printError(errorCode,output) {
    printf("Error: ");
    fprintf(output, "Error: ");
    if (errorCode == 0) {
        printf("Scanning error detected by lexer (skipsym present)\n");
        fprintf(output,"Scanning error detected by lexer (skipsym present)\n");
    } else if (errorCode == 1) {
        printf("program must end with period\n");
        fprintf(output,"program must end with period\n");
    } else if (errorCode == 2) {
        printf("const, var, and read keywords must be followed by identifier\n");
        fprintf(output,"const, var, and read keywords must be followed by identifier\n");
    } else if (errorCode == 3) {
        printf("symbol name has already been declared\n");
        fprintf(output,"symbol name has already been declared\n");
    } else if (errorCode == 4) {
        printf("constants must be assigned with =\n");
        fprintf(output,"symbol name has already been declared\n");
    } else if (errorCode == 5) {
        printf("constants must be assigned an integer value\n");
        fprintf(output,"symbol name has already been declared\n");
    } else if (errorCode == 6) {
        printf("constant and variable declarations must be followed by a semicolon\n");
        fprintf(output,"constant and variable declarations must be followed by a semicolon\n");
    } else if (errorCode == 7) {
        printf("undeclared identifier\n");
        fprintf(output,"undeclared identifier\n");
    } else if (errorCode == 8) {
        printf("only variable values may be altered\n");
        fprintf(output,"only variable values may be altered\n");
    } else if (errorCode == 9) {
        printf("assignment statements must use :=\n");
        fprintf(output,"assignment statements must use :=\n");
    } else if (errorCode == 10) {
        printf("begin must be followed by end\n");
        fprintf(output,"begin must be followed by end\n");
    } else if (errorCode == 11) {
        printf("if must be followed by then\n");
        fprintf(output,"if must be followed by then\n");
    } else if (errorCode == 12) {
        printf("while must be followed by do\n");
        fprintf(output,"while must be followed by do\n");
    } else if (errorCode == 13) {
        printf("condition must contain comparison operator\n");
        fprintf(output,"condition must contain comparison operator\n");
    } else if (errorCode == 14) {
        printf("right parenthesis must follow left parenthesis\n");
        fprintf(output,"right parenthesis must follow left parenthesis\n");
    } else if (errorCode == 15) {
        printf("arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
        fprintf(output,"arithmetic equations must contain operands, parentheses, numbers, or symbols\n");
    }
}
int main() {
    FILE * input = fopen("input.txt","r");
    FILE * output = fopen("elf.txt", "w");

    if (input) {
        printf("Successfully opened file.\n") // TEMPORARY
    } else {
        printf("Failed to open input file. Did you check the input file name?\n");
        return 0;
    }


    return 0;
}