/*
Assignment:
vm.c - Implement a P-machine virtual P-machine

Authors: Nessar Amiri, Marcelo Pacheco

Language: C (only)

To Compile:
    gcc -02 -Wall -std=c11 -o vm vm.c

To Execute (on Eustis):
    ./vm input.txt

where:
    input.txt is the name of the file containing PM/O instructions;
    each line has three integers (OP L M)

Notes:
    - Implements the PM/O virtual machine described in the homework
      instructions
    - No dynamic memory allocation or pointer arithmetic.
    - Does not implement any VM instruction using a separate function.
    - Runs on Eustis.
Class: COP 3402 - Systems Software - Fall 2025

Instructor : Dr. Jie lin

Due Date: Friday, September 12th, 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAS_SIZE 500
// defines variables
int pas[PAS_SIZE] = {0};
int PC, BP, SP;

struct InstructionRegister {
    int OP;
    int L;
    int M;
};

// defines function prototypes
int base(int BP, int L);
void printStack(int PC, int BP, int SP, int pas);

int main(int argc, char * argv[]) {
    if (argc != 2) {
        fprintf(stderr, "This program only accepts one argument\n");
        return 0;
    }
    struct InstructionRegister IR; // sets up struct

    // reads input and opens
    FILE * fp = fopen(argv[1], "r");

    // if unable to find file, terminates program
    if (fp == NULL) {
        printf("Unable to open file.\n");
        return 0;
    }

    // temp variables
    int op, l, m;
    int index = PAS_SIZE - 1;
    int lastLoadedM = index;

    while (fscanf(fp, "%d %d %d", &op, &l, &m) == 3) {
        pas[index] = op;
        pas[index - 1] = l;
        pas[index - 2] = m;
        lastLoadedM = index - 2;
        index -= 3;
    }

    fclose(fp);

    //initialize the registers
    PC = PAS_SIZE - 1;
    SP = lastLoadedM;       //Last loaded M address
    BP = SP - 1;

    int flag = 0; // flag for halt operation

    while (flag == 0) {
        // fetch cycle
        IR.OP = pas[PC];
        IR.L = pas[PC - 1];
        IR.M = pas[PC - 2];
        PC -= 3;

        switch (IR.OP)
        {
        case 1:     //LIT 0 M
            SP--;
            pas[SP] = IR.M;
            break;
        
        case 2:     //OPR
            switch (IR.M)
            {
            case 0:         //RTN 0 0
                SP = BP + 1;
                BP = pas[SP - 2];
                PC = pas[SP - 3];
                break;

            case 1:         // ADD 0 1
                pas[SP + 1] = pas[SP + 1] + pas[SP];
                SP++;
                break;
            
            case 2:         //SUB 0 2
                pas[SP + 1] = pas[SP + 1] - pas[SP];
                SP++;
                break;

            case 3:         //MUL 0 3
                pas[SP + 1] = pas[SP + 1] * pas[SP];
                SP++;
                break;
            
            case 4:         // DIV 0 4
                pas[SP + 1] = pas[SP + 1] / pas[SP];
                SP++;
                break;

            case 5:         //EQL 0 5
                if (pas[SP + 1] == pas[SP])
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                SP++;
                break;
            
            case 6:         //NEQ 0 6
                if (pas[SP + 1] != pas[SP])
                        pas[SP + 1] = 1;
                    else
                        pas[SP + 1] = 0;
                    SP++;
                    break;

            case 7:         //LSS 0 7
                if (pas[SP + 1] < pas[SP])
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                SP++;
                break;

            case 8:         //LEQ 0 8
                if (pas[SP + 1] <= pas[SP])
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                SP++;
                break;

            case 9:         //GTR 0 9
                if (pas[SP + 1] > pas[SP])
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                SP++;
                break;

            case 10:        //GEQ 0 10
                if (pas[SP + 1] >= pas[SP])
                    pas[SP + 1] = 1;
                else
                    pas[SP + 1] = 0;
                SP++;
                break;
            default:
                fprintf(stderr, "%d is an unknow OPR\n", IR.M);
                return 0;
            }
            break;

        case 3:     //LOD L M
            SP--;
            pas[BP] = pas[base(BP, IR.L) - IR.M];
            break;

        case 4:     //STO L M
            pas[base(BP, IR.L) - IR.M] = pas[SP];
            SP++;
            break;

        case 5:     //CAL L M
            pas[SP - 1] = base(BP, IR.L);
            pas[SP - 2] = BP;
            pas[SP - 3] = PC;
            BP = SP - 1;
            PC = (PAS_SIZE - 1) - IR.M;         //this is due to downward implementation
            break;

        case 6:     //IN 0 M
            SP -= IR.M;
            break;

        case 7:     //JMP 0 M
            PC = (PAS_SIZE - 1) - IR.M;
            break;

        case 8:     //JPC 0 M
            if(pas[SP] == 0)
                PC = (PAS_SIZE - 1) - IR.M;
            SP++;
            break;
        
        case 9:     //SYS
            switch(IR.M) {
                case 1:     //SYS 0 1
                    printf("Output result is : %d\n", pas[SP]);
                    SP++;
                    break;
                
                case 2:     //SYS 0 2
                    int val;
                    printf("Please Enter an Integer : ");
                    fflush(stdout);
                        if (scanf("%d", &val) != 1) {
                            fprintf(stderr, "Input error\n");
                            return 1;
                        }
                    SP--;
                    pas[SP] = val;

                case 3:     //SYS 0 3
                    flag = 1;
                    break;

                default:
                    fprintf(stderr, "%d is an unkown SYS\n");
                    return 0;
            }
            break;
        }
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

void printStack(int PC, int BP, int SP, int pas) {
    
}
