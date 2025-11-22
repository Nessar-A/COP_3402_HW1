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
    gcc -O2 -std=c11 -o vm.vm.c
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

#include <stdio.h>
#include <stdlib.h>

// defines variables
#define PAS_SIZE 500
int pas[PAS_SIZE] = {0};
int PC, BP, SP;

// defines IR struct
struct InstructionRegister {
    int OP;
    int L;
    int M;
};

// defines function prototypes
int base(int BP, int L);
void printStack(int BP, int SP);

// main
int main(int argc, char * argv[]) {

    if (argc != 2) {
        fprintf(stderr, "This program only accepts one argument\n");
        return 0;
    }

    // reads input and opens
    FILE * fp = fopen(argv[1], "r");

    struct InstructionRegister IR; // sets up struct

    // if unable to find file, terminates program
    if (fp == NULL) {
        printf("Unable to open file.\n");
        return 0;
    }

    // temp variables
    int op, l, m;
    int index = PAS_SIZE - 1;
    int lastLoadedM = index;

    // reads input and assigns to pas
    while (fscanf(fp, "%d %d %d", &op, &l, &m) == 3) {
        pas[index] = op;
        pas[index - 1] = l;
        pas[index - 2] = m;
        lastLoadedM = index - 2;
        index -= 3;
    }

    fclose(fp); // closes file

    //initialize the registers
    PC = PAS_SIZE - 1;
    SP = lastLoadedM; // last loaded M address
    BP = SP - 1;

    int flag = 0; // flag for halt operation

    printf("        L       M    PC   BP   SP   stack\n");
    printf("Initial values:      %d  %d  %d\n", PC, BP, SP);

    while (flag == 0) {
        // fetch cycle
        IR.OP = pas[PC];
        IR.L = pas[PC - 1];
        IR.M = pas[PC - 2];
        PC -= 3;
        switch (IR.OP)
        {
        case 1: // LIT
            SP--;
            pas[SP] = IR.M;
            printf("LIT     ");
            break;
        
        case 2: // OPR
            switch (IR.M)
            {
            case 0: // RTN
                SP = BP + 1;
                BP = pas[SP - 2];
                PC = pas[SP - 3];
                printf("RTN     ");
                break;

            case 1: // ADD
                pas[SP + 1] = pas[SP + 1] + pas[SP];
                SP++;
                printf("ADD     ");
                break;
            
            case 2: // SUB
                pas[SP + 1] = pas[SP + 1] - pas[SP];
                SP++;
                printf("SUB     ");
                break;

            case 3: // MUL
                pas[SP + 1] = pas[SP + 1] * pas[SP];
                SP++;
                printf("MUL     ");
                break;
            
            case 4: // DIV
                pas[SP + 1] = pas[SP + 1] / pas[SP];
                SP++;
                printf("DIV     ");
                break;

            case 5: // EQL
                if (pas[SP + 1] == pas[SP])
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                SP++;
                printf("EQL     ");
                break;
            
            case 6: // NEQ
                if (pas[SP + 1] != pas[SP])
                        pas[SP + 1] = 1;
                    else
                        pas[SP + 1] = 0;
                    SP++;
                    printf("NEQ     ");
                    break;

            case 7: // LSS  
                if (pas[SP + 1] < pas[SP])
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                SP++;
                printf("LSS     ");
                break;

            case 8: // LEQ
                if (pas[SP + 1] <= pas[SP])
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                SP++;
                printf("LEQ     ");
                break;

            case 9: // GTR
                if (pas[SP + 1] > pas[SP])
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                SP++;
                printf("GTR     ");
                break;

            case 10: // GEQ
                if (pas[SP + 1] >= pas[SP])
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                SP++;
                printf("GEQ     ");
                break;
            case 11: // EVEN
                if (pas[SP] % 2 == 0)
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                printf("EVEN     ");
                break;
            default:
                fprintf(stderr, "%d is an unknown OPR\n", IR.M);
                return 0;
            }
            break;

        case 3: // LOD
            SP--;
            pas[SP] = pas[base(BP, IR.L) - IR.M];
            printf("LOD     ");
            break;

        case 4: // STO
            pas[base(BP, IR.L) - IR.M] = pas[SP];
            SP++;
            printf("STO     ");
            break;

        case 5: // CAL
            pas[SP - 1] = base(BP, IR.L);
            pas[SP - 2] = BP;
            pas[SP - 3] = PC;
            BP = SP - 1;
            PC = (PAS_SIZE - 1) - IR.M;
            printf("CAL     ");
            break;

        case 6: // INC
            SP -= IR.M;
            printf("INC     ");
            break;

        case 7: // JMP
            PC = (PAS_SIZE - 1) - IR.M;
            printf("JMP     ");
            break;

        case 8:  // JPC
            if(pas[SP] == 0)
                PC = (PAS_SIZE - 1) - IR.M;
            SP++;
            printf("JPC     ");
            break;
        
        case 9: // SYS
            switch(IR.M) {
                case 1: // SYS 0 1
                    printf("Output result is: %d\n", pas[SP]);
                    SP++;
                    printf("SYS     ");
                    break;
                
                case 2: // SYS 0 2
                    int val;
                    printf("Please Enter an Integer: ");
                    scanf("%d", &val);
                    SP--;
                    printf("SYS     ");
                    pas[SP] = val;
                    break;

                case 3: // SYS 0 3
                    flag = 1;
                    printf("SYS     ");
                    break;
            }
            break;
        }
        // prints line
        printf("%d       %d    %d  %d  %d  ", IR.L, IR.M, PC, BP, SP); // prints L, M, PC, BP, and SP
        printStack(BP, SP); // prints the stack
        printf("\n"); // prints new-line
    }

    return 0;
}

int base(int BP, int L) {
    int arb = BP;
    
    while (L > 0) {
        arb = pas[arb];
        L--;
    }
    
    return arb;
}

void printStack(int BP, int SP) {
    if (pas[BP] != 0) { // recursively call if it's not equal to 0
        printStack(pas[BP], BP + 1);
    }

    if (BP > SP) {
        if (pas[BP] != 0) {
            printf("| ");
        }
        
        // loop to display stack
        for (int i = BP; i >= SP; i--) {
            printf("%d ", pas[i]);
        }
    }
    
}
