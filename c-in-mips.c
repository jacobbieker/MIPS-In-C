#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// The last 0 in the RegisterFile is for the registers $0, $zero
static int RegisterFile[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
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


//ALU function

int alu(int operandA, operandB, char Operation) {
	int result;
	if (Operation == "add") {
		
	}
	else if (Operation == "addi") {

	}
	else if (Operation == "addu") {

	}
	else if (Operation == "addiu") {

	}
	else if (Operation == "sub") {

	}
	else if (Operation == "subu") {

	}
	else if (Operation == "and") {

	}
	else if (Operation == "andi") {

	}
	else if (Operation == "or") {

	}
	else if (Operation == "ori") {

	}
	else if (Operation == "xor") {

	}
	else if (Operation == "nor") {

	}
	else if (Operation == "slt") {

	}
	else if (Operation == "slti") {

	}
	else if (Operation == "sltu") {

	}
	else {
		perror("Command Not Found");
		exit(-1);
	}
	return result;
}

// Start of Arithmetic functions
//TODO: Mult and Divide, all operations with 'u' have to use unsigned values, others ahve to check for overflow
int add(register1, register2, register3) {
	register1 = RegisterFile[register2] + RegisterFile[register3];
	return register1;
}

unsigned int addu(register1, register2, register3) {
	register1 = RegisterFile[register2] + RegisterFile[register3];
	return register1;
}

int sub(register1, register2, register3) {
	register1 = RegisterFile[register2] - RegisterFile[register3];
	return register1;
}

unsigned int subu(register1, register2, register3) {
	register1 = RegisterFile[register2] - RegisterFile[register3];
	return register1;
}

int addi(register1, register2, number) {
	register1 = RegisterFile[register2] + number;
	return register1;
}

unsigned int addiu(register1, register2, number) {
	register1 = RegisterFile[register2] + number;
	return register1;
}

//Mult and Divide left, have to use LO and HI

// Start of Logical functions

int and(register1, register2, register3) {
	if (RegisterFile[register2] == 1 && RegisterFile[register3] == 1) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
	return register1;
}

int andi(register1, register2, number) {
	if (RegisterFile[register2] == 1 && number == 1) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
	return register1;
}

int or(register1, register2, register3) {
	if (RegisterFile[register2] == 1 || RegisterFile[register3] == 1) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
	return register1;
}

int ori(register1, register2, number) {
	if (RegisterFile[register2] == 1 || number == 1) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
	return register1;
}

int xor(register1, register2, register3) {
	if ((RegisterFile[register2] == 1 && RegisterFile[register3] != 1) || (RegisterFile[register2] != 1 && RegisterFile[register3] == 1)) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
	return register1;
}

int nor(register1, register2, register3) {
	if (!(RegisterFile[register2] == 1 || RegisterFile[register3] == 1)) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
	return register1;
}

int slt(register1, register2, register3) {
	if (RegisterFile[register2] < RegisterFile[register3]) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
	return register1;
}

unsigned int sltu(register1, register2, register3) {
	if (RegisterFile[register2] < RegisterFile[register3]) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
	return register1;
}

int slti(register1, register2, number) {
	if (RegisterFile[register2] < number) {
		register1 = 1;
	}
	else {
		register1 = 0;
	}
	return register1;
}