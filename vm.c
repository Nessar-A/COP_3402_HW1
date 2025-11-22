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

#define PAS_SIZE 500
#define MAX_BARS  200

/* ===== Globals (per instructor note) ===== */
int PAS[PAS_SIZE] = {0};   /* unified PAS: text (top, downward) + stack (downward) */
int PC = 0, BP = 0, SP = 0;

/* Instruction register (expanded form to avoid typedef issues) */
struct instruction {
    int OP;
    int L;
    int M;
};
typedef struct instruction instruction;
instruction IR;

/* For activation-record separators (store SP at time of CAL) */
int bars[MAX_BARS];
int bar_count = 0;

/* Highest stack address currently visible for printing (inclusive). */
int stack_hi = -1;

/* Track whether we've already printed the very first JMP (which omits stack) */
int first_jmp_printed = 0;

/* Follow static link L levels (SL lives at PAS[BP]) */
int base(int BPval, int L) {
    int arb = BPval;
    while (L > 0) {
        arb = PAS[arb]; /* SL */
        L--;
    }
    return arb;
}

/* Print the stack from high to low (descending) with bars only if there are values to the right. */
void print_stack_desc(void) {
    int bar_idx = 0;
    for (int i = stack_hi; i >= SP; --i) {
        printf(" %d", PAS[i]);
        /* Print a '|' only if this bar position is NOT the very last printed element.
           (Avoid trailing bar on CAL line before INC.) */
        if (bar_idx < bar_count && i == bars[bar_idx] && i > SP) {
            printf(" |");
            bar_idx++;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: expected exactly one argument (input file)\n");
        return 1;
    }

    /* ===== Load program into PAS, from 499 downward in triples (OP,L,M) ===== */
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror("Unable to open input file"); return 1; }

    int op, L, M;
    int store = PAS_SIZE - 1; /* 499 */
    int lowest_used = store;
    int triples = 0;

    while (fscanf(f, "%d %d %d", &op, &L, &M) == 3) {
        if (store < 2) { fprintf(stderr, "Error: program too large for PAS\n"); fclose(f); return 1; }
        PAS[store] = op;     /* OP */
        PAS[store - 1] = L;  /* L  */
        PAS[store - 2] = M;  /* M  */
        lowest_used = store - 2;
        store -= 3;
        triples++;
    }
    fclose(f);
    if (triples == 0) { fprintf(stderr, "Error: no instructions loaded\n"); return 1; }

    /* ===== Init registers per spec ===== */
    PC = PAS_SIZE - 1; /* 499 */
    SP = lowest_used;  /* address of last M loaded */
    BP = SP - 1;
    stack_hi = SP - 1; /* nothing on stack yet */

    /* Header + initial line (exact phrasing/spaces) */
    printf("L M PC BP SP stack\n");
    printf("Initial values : %d %d %d\n", PC, BP, SP);

    int running = 1;

    while (running) {
        /* ===== FETCH ===== */
        IR.OP = PAS[PC];
        IR.L  = PAS[PC - 1];
        IR.M  = PAS[PC - 2];
        PC -= 3;

        switch (IR.OP) {
            case 1: { /* LIT 0 M : push literal */
                SP--;
                PAS[SP] = IR.M;
                if (stack_hi < SP) stack_hi = SP;
                printf("LIT %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                print_stack_desc();
                printf("\n");
                break;
            }

            case 2: { /* OPR 0 M : operations / return */
                if (IR.M == 0) { /* RTN */
                    /* Use DL/RA from the current AR:
                       RA at PAS[BP - 2], DL at PAS[BP - 1].
                       Then set SP <- oldBP + 1, BP <- DL, PC <- RA.
                    */
                    int oldBP = BP;
                    int RA = PAS[oldBP - 2];
                    int DL = PAS[oldBP - 1];
                    SP = oldBP + 1;
                    BP = DL;
                    PC = RA;
                    if (bar_count > 0) bar_count--;
                    /* Do not shrink stack_hi (keep caller's previous contents visible) */
                    printf("RTN %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                    print_stack_desc();
                    printf("\n");
                } else {
                    const char *mn = "";
                    switch (IR.M) {
                        case 1: PAS[SP+1] = PAS[SP+1] + PAS[SP]; SP++; mn = "ADD"; break;
                        case 2: PAS[SP+1] = PAS[SP+1] - PAS[SP]; SP++; mn = "SUB"; break;
                        case 3: PAS[SP+1] = PAS[SP+1] * PAS[SP]; SP++; mn = "MUL"; break;
                        case 4: PAS[SP+1] = PAS[SP+1] / PAS[SP]; SP++; mn = "DIV"; break;
                        case 5: PAS[SP+1] = (PAS[SP+1] == PAS[SP]) ? 1 : 0; SP++; mn = "EQL"; break;
                        case 6: PAS[SP+1] = (PAS[SP+1] != PAS[SP]) ? 1 : 0; SP++; mn = "NEQ"; break;
                        case 7: PAS[SP+1] = (PAS[SP+1] <  PAS[SP]) ? 1 : 0; SP++; mn = "LSS"; break;
                        case 8: PAS[SP+1] = (PAS[SP+1] <= PAS[SP]) ? 1 : 0; SP++; mn = "LEQ"; break;
                        case 9: PAS[SP+1] = (PAS[SP+1] >  PAS[SP]) ? 1 : 0; SP++; mn = "GTR"; break;
                        case 10: PAS[SP+1] = (PAS[SP+1] >= PAS[SP]) ? 1 : 0; SP++; mn = "GEQ"; break;
                        case 11: PAS[SP] = (PAS[SP] % 2 == 0) ? 1 : 0; mn = "EVEN"; break; 
                        default: fprintf(stderr, "Runtime Error: unknown OPR %d\n", IR.M); return 1;
                    }
                    if (stack_hi < SP) stack_hi = SP;
                    printf("%s %d %d %d %d %d", mn, IR.L, IR.M, PC, BP, SP);
                    print_stack_desc();
                    printf("\n");
                }
                break;
            }

            case 3: { /* LOD L M : push value from base(BP,L)-M */
                SP--;
                PAS[SP] = PAS[ base(BP, IR.L) - IR.M ];
                if (stack_hi < SP) stack_hi = SP;
                printf("LOD %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                print_stack_desc();
                printf("\n");
                break;
            }

            case 4: { /* STO L M : store top into base(BP,L)-M ; pop */
                PAS[ base(BP, IR.L) - IR.M ] = PAS[SP];
                SP++;
                printf("STO %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                print_stack_desc();
                printf("\n");
                break;
            }

            case 5: { /* CAL L M : call procedure at code address M */
                /* Create AR: SL, DL, RA (do NOT change SP here) */
                PAS[SP - 1] = base(BP, IR.L); /* SL at BP after update */
                PAS[SP - 2] = BP;             /* DL */
                PAS[SP - 3] = PC;             /* RA */
                BP = SP - 1;
                PC = (PAS_SIZE - 1) - IR.M;   /* address encoding */
                /* Mark the boundary between future locals and these links */
                if (bar_count < MAX_BARS) bars[bar_count++] = SP;
                /* For CAL line, show current stack up through SP only (no AR yet) */
                if (stack_hi < SP) stack_hi = SP;
                printf("CAL %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                print_stack_desc(); /* no trailing bar because we suppress bar at i == SP */
                printf("\n");
                break;
            }

            case 6: { /* INC 0 M : allocate M locals (zero them without touching SL/DL/RA) */
                int oldSP = SP;
                SP -= IR.M;

                /* Locals live at indices [SP .. BP-3]. Never overwrite SL (BP), DL (BP-1), RA (BP-2). */
                int locals_hi = BP - 3;
                if (locals_hi >= SP) {
                    for (int i = SP; i <= locals_hi; ++i) PAS[i] = 0;
                }

                /* After INC, include links too so bar becomes visible */
                if (stack_hi < BP) stack_hi = BP;

                printf("INC %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                print_stack_desc();
                printf("\n");
                break;
            }

            case 7: { /* JMP 0 M : jump to address */
                PC = (PAS_SIZE - 1) - IR.M;
                if (!first_jmp_printed) {
                    /* First JMP (at program start) prints registers only */
                    printf("JMP %d %d %d %d %d\n", IR.L, IR.M, PC, BP, SP);
                    first_jmp_printed = 1;
                } else {
                    /* Subsequent JMPs print stack as well */
                    printf("JMP %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                    print_stack_desc();
                    printf("\n");
                }
                break;
            }

            case 8: { /* JPC 0 M : conditional jump if top==0 ; pop */
                if (PAS[SP] == 0) PC = (PAS_SIZE - 1) - IR.M;
                SP++;
                printf("JPC %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                print_stack_desc();
                printf("\n");
                break;
            }

            case 9: { /* SYS 0 M */
                if (IR.M == 1) {
                    /* Output then pop */
                    printf("Output result is : %d\n", PAS[SP]);
                    SP++;
                    printf("SYS %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                    print_stack_desc();
                    printf("\n");
                } else if (IR.M == 2) {
                    /* Read integer from user (type 8 to match your golden trace) */
                    int val;
                    printf("Please Enter an Integer : ");
                    fflush(stdout);
                    if (scanf("%d", &val) != 1) {
                        fprintf(stderr, "Input error\n");
                        return 1;
                    }
                    SP--;
                    PAS[SP] = val;
                    if (stack_hi < SP) stack_hi = SP;
                    printf("SYS %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                    print_stack_desc();
                    printf("\n");
                } else if (IR.M == 3) {
                    printf("SYS %d %d %d %d %d", IR.L, IR.M, PC, BP, SP);
                    print_stack_desc();
                    printf("\n");
                    running = 0;
                } else {
                    fprintf(stderr, "Runtime Error: unknown SYS %d\n", IR.M);
                    return 1;
                }
                break;
            }

            default:
                fprintf(stderr, "Runtime Error: unknown opcode %d\n", IR.OP);
                return 1;
        }
    }

    return 0;
}
