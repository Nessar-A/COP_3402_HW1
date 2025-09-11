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

// defines variables
int pas[500];
int PC, BP, SP;
struct InstructionRegister {
    int OP;
    int L;
    int M;
}

// defines function prototypes
int base(int BP, int L);
void printStack(int PC, int BP, int SP, int pas);

int main() {
    struct InstructionRegister IR;
    

    for (int i = 0; i < 500; i++) {
        pas[i] = 0;
    }

    IR.OP = pas[PC];
    IR.L = pas[PC - 1];
    IR.M = pas[PC - 2];

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