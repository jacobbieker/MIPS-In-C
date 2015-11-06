#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

static int RegisterFile[31] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char RegisterFileNames[31][4] = {"$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$gp", "$sp", "$fp", "$ra"};
static int data_memory[1024];
static char* string_memory[1024];
static char* instructions[1000000];

int main() {
    int i=0;
    readmips("bubble.asm");
    while(instructions[i]!=NULL){
        printf("%s\n",instructions[i]);
        ++i;
    }
    while(instructions[i]!=NULL){
        free(instructions[i]);
        i++;
    }
    return 0;
}

int readmips(char* filename){
    FILE * assembly = fopen(filename, "r");
    int lineno=0; int charno=0; int i =0; int spacebreak=0;
    char buffer[256];
    char* temp;
    if (assembly == 0)
    {
        //fopen returns 0, the NULL pointer, on failure
        perror("Cannot open input file\n");
        exit(-1);
    }
    else {
        while(fgets(buffer,256,assembly)!=NULL){
            charno=0;
            while(buffer[charno]!='#' && buffer[charno]!='\n'){
                charno++;
            } 
            temp = (char*)malloc(charno+1);
            i=0; spacebreak=0;
            while(isspace(buffer[i])){
                i++;
            }
            spacebreak=i;
            while(i<charno){
                *(temp+i-spacebreak)=buffer[i];
                i++;
            }
            //printf("%s\n",temp);
            instructions[lineno]=temp;
            if (charno!=0){
                lineno++;
            }
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