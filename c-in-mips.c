#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// The first and (probably) only comment.... :DDD

static int RegisterFile[31] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char RegisterFileNames[31][4] = {"$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$gp", "$sp", "$fp", "$ra"};
static int data_memory[1024];
static char* string_memory[2014];
static char* instructions[1000000];

int main() {

    return 0;
}

int readmips(char* filename){
    FILE * assembly = fopen(filename, "r");
    int i=0;
    if (assembly == 0) {
        //fopen returns 0, the NULL pointer, on failure
        perror("Cannot open input file\n");
        exit(-1);
    }
    else
    {
        while(fscanf(assembly,"%[#\n]",instructions[i])!=EOF){
            i++;
            printf("%s", instructions[i]);
        }
    }
}

// Start of Arithmetic functions
//TODO: Mult and Divide, all operations with 'u' have to use unsigned values, others ahve to check for overflow
void add(register1, register2, register3) {
	register1 = register2 + register3;
}

void addu(register1, register2, register3) {
	register1 = register2 + register3;
}

void sub(register1, register2, register3) {
	register1 = register2 - register3;
}

void subu(register1, register2, register3) {
	register1 = register2 - register3;
}

void addi(register1, register2, number) {
	register1 = register2 + number;
}

void addiu(register1, register2, number) {
	register1 = register2 + number;
}

//Mult and Divide left, have to use LO and HI

// Start of Logical functions

void and(register1, register2, register3) {
	if (register2 == 1 && register3 == 1) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
}

void andi(register1, register2, number) {
	if (register2 == 1 && number == 1) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
}

void or (register1, register2, register3) {
	if (register2 == 1 || register3 == 1) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
}

void ori(register1, register2, number) {
	if (register2 == 1 || number == 1) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
}

void xor(register1, register2, register3) {
	if ((register2 == 1 && register3 != 1) || (register2 != 1 && register3 == 1)) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
}

void nor(register1, register2, register3) {
	if (!(register2 == 1 || register3 == 1)) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
}

void slt(register1, register2, register3) {
	if (register2 < register3) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
}

void sltu(register1, register2, register3) {
	if (register2 < register3) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
}

void slti(register1, register2, number) {
	if (register2 < number) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
}