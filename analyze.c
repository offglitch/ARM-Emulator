#include <stdio.h>

int sum_array_a(int *array, int n);
int find_max_a(int *array, int n);
int fib_rec_a(int n);
int fib_iter_a(int n);
int strlen_a(char *s, char *sub);

int analyze_iw(unsigned int iw)
{
    unsigned int opcode, rn, rd, rm, imm;
    
    /* Show hex representation of iw. */
    printf("iw = %X\n", iw);

    /* Extract the opcode. */
    opcode = (iw >> 21) & 0b1111;

    printf("opcode = %d\n", opcode);

    /* Extract the register operand numbers. */
    rn = (iw >> 16) & 0b1111;
    rd = (iw >> 12) & 0b1111;
    rm = iw & 0b1111;
    imm = iw & 0b11111111;
    
    printf("rn = %d\n", rn);
    printf("rd = %d\n", rd);
    printf("rm = %d\n", rm);
    printf("imm = %d\n", imm);
}

int main(int argc, char **argv)
{
    unsigned int iw;
    unsigned int *pc;

    /* Get the address of the first instruction of the add assembly
       function in memory. */
    pc = (unsigned int *) fib_rec_a;
    printf("pc = %X\n", (unsigned) pc);
    
    /* Get the 32 bit instruction work at the pc address. */
    iw = *pc;
    analyze_iw(iw);

    /* Move to next instruction */
    pc = pc + 1;
    printf("pc = %X\n", (unsigned) pc);
    
    iw = *pc;
    analyze_iw(iw);

    return 0;
}
