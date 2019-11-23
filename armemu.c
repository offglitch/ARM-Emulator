#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#define NREGS 16
#define STACK_SIZE 1024
#define SP 13
#define LR 14
#define PC 15
#define ADDRESS_BITS 32

/* Counters for instructions */
int iw_i;
int dp_i;
int b_i;
int mem_i;

/* Cache size - use default of 128 bytes */
int c = 128;

int quadratic_c(int x, int a, int b, int c);
int quadratic_a(int x, int a, int b, int c);
int sum_array_c(int *array, int n);
int sum_array_a(int *array, int n);
int find_max_c(int *array, int n);
int find_max_a(int *array, int n);
int fib_iter_c(int n);
int fib_iter_a(int n);
int fib_rec_c(int n);
int fib_rec_a(int n);
int strlen_c(char *s);
int strlen_a(char *s);

/* 
    N - Cache size in Bytes
    A - Associativity (1)
    B - Block Size (4 bytes)
*/
int N, A, B;
int cache_hit;
int cache_miss;
int cache_requests;
int index_mask = 1;

struct cache_bits {
    int tag_bits;
    int index_bits;
    int offset_bits;
} cache_bits_size;

unsigned int address;
int size;

/* Initilize cache RAMs struct */
struct cache_rams_struct {
    unsigned int tag_ram[63][2048];
    //Valid bit RAM 1-bit for every set in every way
    int valid_ram[63][2048];
} cache_rams;


struct arm_state {
    unsigned int regs[NREGS];
    unsigned int cpsr;
    unsigned char stack[STACK_SIZE];
};

/* Function to calculate the log to the base 2 of a given number */
int num_bits(int num) {
    int count = 0;
    while (num) {
        num = num>>1;
        count++;
    }
    return count-1;
}

/* Function to initialize RAMs */ 
void init_rams(int A, int S) {
    int i, j;
    for (i = 0; i < A; i++) {
        for (j = 0; j < (S/A); j++) {
            cache_rams.valid_ram[i][j] = 0;
        }
    }
}

/* Looks up the tag ram and reports a hit if the tag bits of the address match in the tag ram */
int cache_lookup(unsigned int address, int A, int index) {
    int i;
    int lookup = 0;
    address = address >> (cache_bits_size.offset_bits + cache_bits_size.index_bits);
    for (i = 0; i < A; i++) {
    //printf("tag_addr: %x tag: %x\t valid: %d\tindex: %d\n", address, cache_rams.tag_ram[i][index], cache_rams.valid_ram[i][index], index);
        if ((address == cache_rams.tag_ram[i][index]) && (cache_rams.valid_ram[i][index])) {
            lookup = 1;
            return lookup;
        }
    }
    return lookup;
}

/* Determines if set in cache is full */
int is_cache_index_full(int A, int index) {
    int i;
    for (i = 0; i < A; i++) {
        if (cache_rams.valid_ram[i][index] != 1) {
        //if the valid bit isn't set for any of the way in that particular set, then that particular
        // set in the cache isn't full
        return 0;
        }
    }
    return 1;
}

int cache_init(int c) {
    char *p;
    N = c;
    A = 1;
    B = 4;
    int S = 0;
    //Calculate the total number of sets
    //S = cache_size/block_size
    S = (int) (N/B);
    init_rams(A, S);
    //Cache size is the bits required
    //for tag, index and offset fields
    char operation;
    int iter;
    char valid;
    //number of bits in offset is equal
    //to the bits required to map the size 
    //of a cache block/line
    cache_bits_size.offset_bits = num_bits(B);
    //number of bits in index is equal
    //to the bits required to map the number
    //of sets in (one way if associative) the cache
    cache_bits_size.index_bits  = num_bits((int)S/A);
    //Finally the number of bits in the tag
    //can be calculated by subtracting the bits used
    //yet from the number of bits in the address
    cache_bits_size.tag_bits = ADDRESS_BITS - (cache_bits_size.offset_bits + cache_bits_size.index_bits);
    for(iter = 0; iter < cache_bits_size.index_bits; iter++) {
        index_mask = index_mask<<1;
    }
    index_mask = index_mask - 1;
    //printf ("OFFSET:%d\tINDEX:%d\tTAG:%d\t\n",cache_bits_size.offset_bits, cache_bits_size.index_bits, cache_bits_size.tag_bits); 
    cache_hit = 0;
    cache_miss = 0;
    cache_requests = 0;
}

int cache_op(unsigned int address) {
    int requests = 0;
    int lookup = 0;
    int index = 0;
    index = (address>>cache_bits_size.offset_bits)&index_mask;
    lookup = cache_lookup(address, A, (address>>cache_bits_size.offset_bits)&index_mask);
    if (lookup == 1) {
      cache_hit++;
    }
    else {
      cache_miss++;
      requests = is_cache_index_full(A, (address>>cache_bits_size.offset_bits)&index_mask);
      address = address >> (cache_bits_size.offset_bits + cache_bits_size.index_bits);
      cache_rams.valid_ram[0][index] = 1;
      cache_rams.tag_ram[0][index] = address;
      if (requests) {
        cache_requests++;
        cache_rams.tag_ram[0][index] = address;
      }
    }
}

void print_cache_stats() {
    printf("\nHits:%d\t\tMisses:%d\tRequests:%d\n", cache_hit, cache_miss, cache_requests);
}

/* Initialize an arm_state struct with a function pointer and arguments */
void arm_state_init(struct arm_state *as, unsigned int *func,
                    unsigned int arg0, unsigned int arg1,
                    unsigned int arg2, unsigned int arg3)
{
    int i;

    /* Zero out all registers */
    for (i = 0; i < NREGS; i++) {
        as->regs[i] = 0;
    }

    /* Zero out CPSR */
    as->cpsr = 0;

    /* Zero out the stack */
    for (i = 0; i < STACK_SIZE; i++) {
        as->stack[i] = 0;
    }

    /* Initialise the cache */
    cache_init (c);

    /* Set the PC to point to the address of the function to emulate */
    as->regs[PC] = (unsigned int) func;

    /* Set the SP to the top of the stack (the stack grows down) */
    as->regs[SP] = (unsigned int) &as->stack[STACK_SIZE];

    /* Initialize LR to 0, this will be used to determine when the function has called bx lr */
    as->regs[LR] = 0;

    /* Initialize the first 4 arguments */
    as->regs[0] = arg0;
    as->regs[1] = arg1;
    as->regs[2] = arg2;
    as->regs[3] = arg3;
}

void arm_state_print(struct arm_state *as)
{
    int i;
    printf("---- Registers & CPSR ----\n");
    for (i = 0; i < NREGS; i++) {
        printf("reg[%d] = 0x%x\n", i, as->regs[i]);
    }
    printf("cpsr = %x\n", as->cpsr);
}

void stack_print(struct arm_state *as)
{
    int i;
    printf("---- Stack ----");
    for(i = 0; i<STACK_SIZE; i++){
        printf("stack[%d] = %d\n",i,as->stack[i]);
    }
}

bool is_EQ(struct arm_state *state) 
{
    if (state->cpsr == 0b0000) {
        return true;
    } else {
        return false;
    }
}

bool is_LT(struct arm_state *state) 
{
    if (state->cpsr == 0b0001) {
        return true;
    } else {
        return false;
    }
}

bool is_GT(struct arm_state *state) 
{
    if (state->cpsr == 0b0011) {
        return true;
    } else {
        return false;
    }
}

bool check_cond(struct arm_state *state) 
{
    unsigned int iw, cond, rd;

    iw = *((unsigned int *) state->regs[PC]);

    cond = (iw >> 28) & 0xF;
    if (cond == 0b0000) {
        return is_EQ(state);
    } else if (cond == 0b0001) {
        return !is_EQ(state);
    } else if (cond == 0b1100) {
        return is_GT(state);
    } else if (cond == 0b1101) {
        return is_EQ(state) | is_LT(state);
    } else if (cond == 0b1011) {
        return is_LT(state);
    }

    return true;
}

bool is_add_inst(unsigned int iw)
{
    unsigned int op;
    unsigned int opcode;

    op = (iw >> 26) & 0b11;
    opcode = (iw >> 21) & 0b1111;

    return (op == 0) && (opcode == 0b0100);
}

bool is_sub_inst(unsigned int iw) 
{
    unsigned int op;
    unsigned int opcode;

    op = (iw >> 26) & 0b11;
    opcode = (iw >> 21) & 0b1111;

    return (op == 0) && (opcode == 0b0010);
}

bool is_mov_inst(unsigned int iw) 
{
    unsigned int op;
    unsigned int opcode;
    op = (iw >> 26) & 0b11;
    opcode = (iw >> 21) & 0b1111;

    return (op == 0) && (opcode == 0b1101);
}

bool is_mul_inst( unsigned int iw){
    unsigned int msb, lsb;
    msb = (iw >> 22) & 0b111111;
    lsb = (iw >> 4) & 0xF;

    return (msb == 0) && (lsb == 0b1001);
}

bool is_cmp_inst(unsigned int iw) 
{
    unsigned int op;
    unsigned int opcode;

    op = (iw >> 26) & 0b11;
    opcode = (iw >> 21) & 0b1111;

    return (op == 0) && (opcode == 0b1010);
}

bool is_bx_inst(unsigned int iw)
{
    unsigned int bx_code;

    bx_code = (iw >> 4) & 0x00FFFFFF;

    return (bx_code == 0b000100101111111111110001);
}

bool is_ldr_inst(unsigned int iw) {

    unsigned int opcode;
    unsigned int l_bit;
    unsigned int b_bit;

    opcode = (iw >> 26) & 0b11;
    l_bit = (iw >> 20) & 0b1;
    b_bit = (iw >> 22) & 0b1;

    return (opcode == 1) && (l_bit == 0b1) && (b_bit == 0b0);
}

bool is_ldrb_inst(unsigned int iw) 
{

    unsigned int opcode;
    unsigned int l_bit;
    unsigned int b_bit;

    opcode = (iw >> 26) & 0b11;
    l_bit = (iw >> 20) & 0b1;
    b_bit = (iw >> 22) & 0b1;

    return (opcode == 1) && (l_bit == 0b1) && (b_bit == 0b1);
}

bool is_str(unsigned int iw) 
{
    unsigned int opcode;
    unsigned int l_bit;
    unsigned int b_bit;

    opcode = (iw >> 26) & 0b11;
    l_bit = (iw >> 20) & 0b1;
    b_bit = (iw >> 22) & 0b1;

    return (opcode == 1) && (l_bit == 0b0) && (b_bit == 0b0);
}

bool is_b_inst(unsigned int iw)
{
    unsigned int op;
    unsigned int opcode;

    op = (iw >> 26) & 0b11;
    opcode = (iw >> 21) & 0b1111;

    return (op == 0b10);
}

void armemu_str(struct arm_state *state) 
{
    unsigned int iw;
    unsigned int rd, rn, rm;

    iw = *((unsigned int *) state->regs[PC]);

    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xF;

    *((unsigned int *) state->regs[rn]) = state->regs[rd];

    if (rd != PC) {
        state->regs[PC] = state->regs[PC] + 4;
    }
}

void armemu_add(struct arm_state *state)
{
    unsigned int iw;
    unsigned int rd, rn, rm, immediate, imm8;

    iw = *((unsigned int *) state->regs[PC]);

    immediate = (iw >> 25) & 0b1;

    if (immediate == 0b1) {
        rd = (iw >> 12) & 0xF;
        rn = (iw >> 16) & 0xF;
        imm8 = iw & 0xFF;

        state->regs[rd] = state->regs[rn] + imm8;

    } else {
    
        rd = (iw >> 12) & 0xF;
        rn = (iw >> 16) & 0xF;
        rm = iw & 0xF;

        state->regs[rd] = state->regs[rn] + state->regs[rm];
    }
    if (rd != PC) {
        state->regs[PC] = state->regs[PC] + 4;
    }
}



void armemu_sub(struct arm_state *state) 
{
    //printf("RUNNING SUB\n");
    unsigned int iw;
    unsigned int rd, rn, rm, immediate, imm8;

    iw = *((unsigned int *) state->regs[PC]);
    immediate = (iw >> 25) & 0b1;

    if (immediate == 0b1) {
        rd = (iw >> 12) & 0xF;
        rn = (iw >> 16) & 0xF;
        imm8 = iw & 0xFF;
        state->regs[rd] = state->regs[rn] - imm8;

    } else {
    
        rd = (iw >> 12) & 0xF;
        rn = (iw >> 16) & 0xF;
        rm = iw & 0xF;

        state->regs[rd] = state->regs[rn] - state->regs[rm];
    }
    if (rd != PC) {
        state->regs[PC] = state->regs[PC] + 4;
    }
}



void armemu_mov(struct arm_state *state) 
{
    unsigned iw, rd, immediate, reg;

    iw = *((unsigned int *) state->regs[PC]);
    rd = (iw >> 12) & 0xF;

    immediate = (iw >> 25) & 0b1;

    if (immediate == 0b1) {
        rd = (iw >> 12) & 0xF;
        reg = iw & 0xFF;

        state->regs[rd] = reg;

    } else {
        rd = (iw >> 12) & 0xF;
        reg = iw & 0xF;
        state->regs[rd] = state->regs[reg];
    }

    if (rd != PC) {
        state->regs[PC] = state->regs[PC] + 4;
    }
}


void armemu_cmp(struct arm_state *state) 
{
    //printf("cmp executing \n");
    //zero out the cpsr
    state->cpsr = 0b0000;
    unsigned iw, r1, immediate, reg, rd;
    iw = *((unsigned int *) state->regs[PC]);
    rd = (iw >> 12) & 0xF;
    immediate = (iw >> 25) & 0b1;

    if (immediate == 0b1) {
        r1 = (int) state->regs[(iw >> 16) & 0b1111];
        reg = iw & 0xFF;
        if (r1 == reg) {
            state->cpsr = 0b0000;
        } else if (r1 < reg) {
            state->cpsr = 0b0001;
        } else if (r1 > reg) {
            state->cpsr = 0b0011;
        }
    } else {
        r1 = (int) state->regs[(iw >> 16) & 0b1111];
        reg = (int) state->regs[iw & 0xF];
        if (r1 == reg) {
            state->cpsr = 0b0000;
        } else if (r1 < reg) {
            state->cpsr = 0b0001;
        } else if (r1 > reg) {
            state->cpsr = 0b0011;
        }
    }

    if (rd != PC) {
        state->regs[PC] = state->regs[PC] + 4;
    }
}

void armemu_bx(struct arm_state *state)
{
    unsigned int iw;
    unsigned int rn;

    iw = *((unsigned int *) state->regs[PC]);
    rn = iw & 0b1111;

    state->regs[PC] = state->regs[rn];
}

void armemu_ldr(struct arm_state *state) 
{
    unsigned int iw;
    unsigned int rd, rn, rm, b_bit;
    iw = *((unsigned int *) state->regs[PC]);
    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xF;

    state->regs[rd] = *((unsigned int *) state->regs[rn]);

    if (rd != PC) {
        state->regs[PC] = state->regs[PC] + 4;
    }
}

void armemu_ldrb(struct arm_state *state) 
{

    unsigned int iw;
    unsigned int rd, rn, rm;

    iw = *((unsigned int *) state->regs[PC]);
    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xF;
    rm = iw & 0xF;

    state->regs[rd] = *((unsigned int *)state->regs[rn]);

    if (rd != PC) {
        state->regs[PC] = state->regs[PC] + 4;
    }
}

void armemu_b(struct arm_state *state)
{
    unsigned int iw;
    signed int immediate;
    unsigned int l_bit;

    iw = *((signed int *) state->regs[PC]);
    immediate = iw & 0xFFFFFF;

    if(((immediate >> 23) & 0b1) == 1){
        immediate = immediate | 0xFF000000;
    }

    immediate = immediate << 2;
    l_bit = (iw >> 24) & 0b1;

    if(l_bit == 0){
        state->regs[PC] = state->regs[PC] + 8 + immediate;
    } else{
        state->regs[LR] = state->regs[PC] + 8 - 4;
        state->regs[PC] = state->regs[PC] + 8 + immediate;
    }
}

void armemu_mul(struct arm_state *state)
{
    unsigned int iw;
    unsigned int rd, rs, rm;
 
    iw = *((unsigned int *) state->regs[PC]);
   
    rd = (iw >> 16) & 0xF;
    rs = (iw >> 8) & 0xF;
    rm = iw & 0xF;
 
    state->regs[rd] = state->regs[rs] * state->regs[rm];

    if (rd != PC) {
        state->regs[PC] = state->regs[PC] + 4;
    }
}

void armemu_one(struct arm_state *state)
{
    unsigned int iw, rd;
    cache_op (state->regs[PC]);
    iw = *((unsigned int *) state->regs[PC]);
    rd = (iw >> 12) & 0xF;

    if (check_cond(state)) {
        if (is_bx_inst(iw)) {
            armemu_bx(state);
            b_i++;
        } else if (is_add_inst(iw)) {
            armemu_add(state);
            dp_i++;
        } else if (is_mul_inst(iw)) {
            armemu_mul(state);
            dp_i++;
        } else if (is_sub_inst(iw)) {
            armemu_sub(state);
            dp_i++;
        } else if (is_mov_inst(iw)) {
            armemu_mov(state);
            dp_i++;
        } else if (is_cmp_inst(iw)) {
            armemu_cmp(state);
            dp_i++;
        } else if (is_ldr_inst(iw)) {
            armemu_ldr(state);
            mem_i++;
        } else if (is_b_inst(iw)) {
            armemu_b(state);
            b_i++;
        } else if (is_str(iw)) {
            armemu_str(state);
            mem_i++;
        } else if (is_ldrb_inst(iw)) {
            armemu_ldrb(state);
            mem_i++;
        }
    } else {
        state->regs[PC] = state->regs[PC] + 4;
    }
}

unsigned int armemu(struct arm_state *state) 
{
    iw_i = 0;
    dp_i = 0;
    mem_i = 0;
    b_i = 0;

    while (state->regs[PC] != 0) {
        iw_i++;
        armemu_one(state);
    }
    return state->regs[0];
}

void test_quad(void)
{
    struct arm_state state;
    unsigned r;
    int quad;

    printf("\n--- Testing Quad ---\n\n");

    arm_state_init(&state, (unsigned int *) quadratic_a, (unsigned int) 2, 4, 6, 8);
    // arm_state_print(&state);
    r = armemu(&state);
    printf("\nTotal of instructions executed = %d, \n\t memory = %d, \n\t data = %d, \n\t branch = %d\n", iw_i, mem_i, dp_i, b_i);
    printf("\n Emulation result for 2, 4, 6, 8 = %d\n", r);
    quad = quadratic_c(2, 4, 6, 8);
    printf("Quadratic Value for 2, 4, 6, 8 in C \t\t\t = %d\n", quad);
    quad = quadratic_a(2, 4, 6, 8);
    printf("Quadratic Value for 2, 4, 6, 8 in Assembly \t\t = %d\n", quad);

    print_cache_stats();

    printf("\n-------------------\n\n");
}

void test_sum_array(void) 
{
    struct arm_state state;
    unsigned r;
    int arr1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    printf("\n--- Testing Sum Array Function ---\n\n");

    arm_state_init(&state, (unsigned int *) sum_array_a, (unsigned int) arr1, 10, 0, 0);
    // arm_state_print(&state);
    r = armemu(&state);
    printf("\nTotal of instructions executed = %d, \n\t memory = %d, \n\t data = %d, \n\t branch = %d\n", iw_i, mem_i, dp_i, b_i);
    printf("\n Emulation result for {1, 2, 3, 4, 5, 6, 7, 8, 9, 10} = %d\n", r);
    printf("\n Assembly result for {1, 2, 3, 4, 5, 6, 7, 8, 9, 10} = %d\n", sum_array_c(arr1, 10));
    printf("\n Assembly result for {1, 2, 3, 4, 5, 6, 7, 8, 9, 10} = %d\n", sum_array_a(arr1, 10));

    print_cache_stats();
    printf("\n-------------------\n\n");
}

void test_find_max(void) 
{
    struct arm_state state;
    unsigned r;
    int arr1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    printf("\n--- Testing Find Max Function ---\n\n");

    arm_state_init(&state, (unsigned int *) find_max_a, (unsigned int) arr1, 10, 0, 0);
    // arm_state_print(&state);
    r = armemu(&state);
    printf("\nTotal of instructions executed = %d, \n\t memory = %d, \n\t data = %d, \n\t branch = %d\n", iw_i, mem_i, dp_i, b_i);
    printf("\n Emulation result for {1, 2, 3, 4, 5, 6, 7, 8, 9, 10} = %d\n", r);
    printf("\n Assembly result for {1, 2, 3, 4, 5, 6, 7, 8, 9, 10} = %d\n", find_max_a(arr1, 10));
    printf("\n C result for {1, 2, 3, 4, 5, 6, 7, 8, 9, 10} = %d\n", find_max_c(arr1, 10));

    print_cache_stats();
    printf("\n-------------------\n\n");
}

void test_fib_iter(void) 
{
    struct arm_state state;
    unsigned r;
    int i;
    int fib;

    printf("\n--- Testing Fibonacci Iterative Function ---\n\n");
    arm_state_init(&state, (unsigned int *) fib_iter_a, (unsigned int) 20, 0, 0, 0);
    // arm_state_print(&state);
    r = armemu(&state);
    printf("\nTotal of instructions executed = %d, \n\t memory = %d, \n\t data = %d, \n\t branch = %d\n", iw_i, mem_i, dp_i, b_i);
    printf("\n Emulation result for 20 = %d\n", r);
    printf("\n Assembly result for 20 = %d\n", fib_iter_a(20));
    printf("\n C result for 20 = %d\n", fib_iter_c(20));

    print_cache_stats();
    printf("\n-------------------\n\n");
}

void test_fib_rec(void) 
{
    struct arm_state state;
    unsigned r;
    int i;

    printf("\n--- Testing Fibonacci Recursive Function ---\n");
    // printf("ASM: %d\n", fib_rec_a(20));
    arm_state_init(&state, (unsigned int *) fib_rec_a, (unsigned int) 20, 0, 0, 0);
    // arm_state_print(&state);
    r = armemu(&state);
    printf("\nTotal of instructions executed = %d, \n\t memory = %d, \n\t data = %d, \n\t branch = %d\n", iw_i, mem_i, dp_i, b_i);
    printf("\n Emulation result for 20 = %d\n", r);
    printf("\n Assembly result for 20 = %d\n", fib_rec_a(20));
    printf("\n C result for 20 = %d\n", fib_rec_c(20));

    print_cache_stats();
    printf("\n-------------------\n\n");
}

void test_strlen(void)
{
    struct arm_state state;
    unsigned r;
    char test_str[20] = "Funky Business!";
    printf("\n--- Testing strlen ---\n");
    arm_state_init(&state, (unsigned int *)strlen_a, (unsigned int)test_str, 0, 0, 0);
    // arm_state_print(&state);
    r = armemu(&state);
    printf("\nTotal of instructions executed = %d, \n\t memory = %d, \n\t data = %d, \n\t branch = %d\n", iw_i, mem_i, dp_i, b_i);
    printf("\n Emulation result for string \"Funky Business!\" = %d\n", r);
    printf("\n Assembly result for string \"Funky Business!\" = %d\n", strlen_a(test_str));
    printf("\n C result for string \"Funky Business!\" = %d\n", strlen_c(test_str));

    print_cache_stats();
    printf("\n-------------------\n\n");
}

int main(int argc, char **argv)
{
    char *p;
    if (strcmp(argv[1], (char *)"-c") == 0) {
      c = strtol(argv[2], &p, 10);
      printf("Cache size is %d\n", c);
    }
    struct arm_state state;
    unsigned r;
    test_quad();
    test_sum_array();
    test_find_max();
    test_fib_iter();
    test_fib_rec();
    test_strlen();

    return 0;
}
