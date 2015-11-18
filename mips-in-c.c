/* CIS 314 Big Project, Week 2 Fall 2015
 *
 *  Authors:
 *      Matthew Jagielski, Jacob Bieker, and Theodore LaGrow
 *
 *  Date:
 *      11/19/2015
 *      
 *  Sources:
 *      CIS 314 on canvas.uoregon.edu
 *      https://www.cs.uoregon.edu/Classes/15F/cis314/
 *      StackOverFlow.com
 *      
 *  Assignment:
 *      Week 1:
 *      Single cycle processor simulation in C: implement (a) a register file, (b) an ALU
 *      (c) control logic, and (d) main memory.
 *
 *      Week 2: (WE ARE HERE!)
 *      Convert the single cycle processor into a pipelined 5-stage processor.
 *
 *      Week 3:
 *      Build a direct-mapped cache structure between the main memory and the 5-stage
 *      processor.
 *  
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <pthread.h>
#define NUM_THREADS	5

static int RegisterFile[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char RegisterFileNames[32][6] = {"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};
static char RegisterNumberNames[32][4] = {"$0", "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15", "$16", "$17", "$18", "$19", "$20", "$21", "$22", "$23", "$24", "$25", "$26", "$27", "$28","$29","$30","$31"};

static int data_memory[1024]; // int values
static char* instructions[1024]; // instructions
static int controlRegisterZ[1024]; // Control RegisterZ

static char* jump_names[128]; // jump location names and addresses
static int jump_locations[128];

static int LO;
static int HI;

// instruction name enum and array
typedef enum {Add, Addu, Sub, Subu, And, Or, Nor, Xor, Slt, Sltu, Srlv, Sllv, Srav, Sll, Srl, Sra, Mult, Multu, MDiv, Divu, MfcZ, MtcZ, Mfhi, Mflo, Jr, Addi, Addiu, Andi, Ori, Slti, Beq, Bne, Lw, Lh, Lhu, Lb, Lbu, Sw, Sh, Sb, Lui, J, Jal, Break} instruction;
static char* instr_name_list[44] = {"add", "addu", "sub", "subu", "and", "or", "nor", "xor", "slt", "sltu", "srlv", "sllv", "srav", "sll", "srl", "sra", "mult", "multu", "div", "divu", "mfcZ", "mtcZ", "mfhi", "mflo", "jr", "addi", "addiu", "andi", "ori", "slti", "beq", "bne", "lw", "lh", "lhu", "lb", "lbu", "sw", "sh", "sb", "lui", "j", "jal", "break"};

struct Instruction_Type { // generic instruction type - holds instruction only
    int *pc_ptr;
    instruction instr;
};

struct r_type { // holds all r type values
    int *pc_ptr;
    instruction instr;
    char dest_register;
    char s_register1;
    char s_register2;
    char shamt;
};

struct i_type { // holds all i type values
    int *pc_ptr;
    instruction instr;
    char dest_register;
    char s_register;
    int immediate;
};

struct j_type{ // holds all j type values
    int *pc_ptr;
    instruction instr;
    int jump_to;
};

struct indexed_register { //for modular parsing of indexed registers (return both index and register)
    char mem_register;
    int index;
};

struct exmem {
    int instrnum;
    int alu_result;
    int dest_register;
    int will_branch;
};

struct memwb {
    int instrnum;
    int value;
    int dest_register;
};

/* readmips(char*) reads the assembly file specified by filename into the instructions data array*/
void readmips(char* filename);

/*getinstruction(int pc) is an interface for the control unit to access the instructions stored
in memory, by index specified by pc*/
char* getinstruction(int pc);

/*decodeinstruction(char*) takes an assembly instruction specified by instr and converts to an
Instruction_Type, converting the function, any registers, and any immediate values to integers
that are easier to operate on*/
struct Instruction_Type* decodeinstruction(char* instr);

/*execute_r(struct r_type*) takes a pointer to an r_type instruction and uses the data within to
execute the instruction and put the correct data into an exmem register struct, which can then be
passed to the memory function*/
struct exmem* execute_r(struct r_type*);

/*execute_i(struct i_type*) takes a pointer to an i_type instruction and uses the data within to
execute the instruction and put the correct data into an exmem register struct, which can then be
passed to the memory function*/
struct exmem* execute_i(struct i_type*);

/*execute_j(struct j_type*, int pc) takes a pointer to a j_type instruction and the value of the program
counter and puts the correct data into an exmem register struct, which can then b passed to the memory function*/
struct exmem* execute_j(struct j_type*, int pc);

/*alu(int val1,int val2,int op) takes two values specified by val1 and val2, and an operation specified by op,
and performs and returns the appropriate value for the operation when performed with the two input values*/
int alu(int, int, int);

/*memory_rw(struct exmem*) takes a pointer to an exmem register struct and uses that to access the memory if needed
and create a memwb struct to pass data on to the register writeback pipeline phase*/
struct memwb* memory_rw(struct exmem*);

/*writeback(struct memwb*) takes a pointer to a memwb register struct and uses its data to write to
the appropriate registers, completing the execution of the instruction. Whew!*/
void writeback(struct memwb memwb_ptr);

/*make_r_type(int, char*, int*) takes the instruction specified by instr, and converts the remainder
of the char* to an r_type struct to hold all the data an r_type can have. instrnum holds the enum
value of the instruction*/
struct r_type* make_r_type(int instrnum, char* instr, int*);

/*make_i_type(int, char*, int*) takes the instruction specified by instr, and converts the remainder
of the char* to an i_type struct to hold all the data an i_type can have. instrnum holds the enum
value of the instruction*/
struct i_type* make_i_type(int instrnum, char* instr, int*);

/*make_j_type(int, char*, int*) takes the instruction specified by instr, and converts the remainder
of the char* to a j_type struct to hold all the data a j_type can have. instrnum holds the enum
value of the instruction*/
struct j_type* make_j_type(int instrnum, char* instr, int*);

/*nextregister(char*, int*) returns the value of the next register in the instruction specified by
instr, past the location specified by *ptr 
returns the index of the register if it is in the list of 31 registers RegisterFileNames, or 31 if
it is the zero register, and 32 if the register was not found*/
char nextregister(char* instr, int* ptr);

/*nextint(char*, int*) returns the next integer found in the instruction specified by instr, past the
location specified by *ptr */
int nextint(char* instr,int* ptr);

/*nextjumploc(char*, int*) returns the next jump location found in the instruction specified by instr, past
the location specified by *ptr. requires the asm file to have been read in using readmips(char*)*/
int nextjumploc(char* instr, int* ptr);

/*nextindexedregister(char*, int*) returns the next indexed register's register and index values from
the instruction specified by instr at the location specified by *ptr */
struct indexed_register* nextindexedregister(char* instr, int* ptr);

// Check for overflow function
/*safe_add(int,int) adds two integers, checking for overflow*/
int safe_add(int a, int b);

/*safe_sub(int,int) subtracts the second integer from the first, checking for overflow*/
int safe_sub(int a, int b);

// Start of Arithmetic functions
//add adds two values
int add(int register2, int register3);

//addu adds unsigned int values
unsigned int addu(int register2, int register3);

// sub subtracts
int sub(int register2, int register3);

// subu subtracts unsigned ints
unsigned int subu(int register2, int register3);

// addi adds an immediate value to a value found in a register
int addi(int register2, int number);

// addiu does the same as addi except with unsigned ints
unsigned int addiu(int register2, int number);

// mult stores the upper half of the product in HI and the lower half in LO
void mult(int register2, int register3);

void multu(int register2, int register3);

void mdiv(int register2, int register3);

void divu(int register2, int register3);

// Start of Logical functions
int and(int register2, int register3);

int andi(int register2, int number);

int or(int register2, int register3);

int ori(int register2, int number);

int xor(int register2, int register3);

int nor(int register2, int register3);

int slt(int register2, int register3);

unsigned int sltu(int register2, int register3);

int slti(int register2, int number);

//Start of Bitwise functions

int sll(int register2, int number);

int srl(int register2, int number);

int sllv(int register2, int register3);

int srlv(int register2, int register3);

int sra(int register2, int number);

int srav(int register2, int register3);

// Start Data Transfer functions headers

// Memory Stage helpers headers

int lw_read(int registerAndIndex);

short lh_read(int registerAndIndex);

void register_write(int register2, int C);

unsigned short lhu_read(int registerAndIndex);

char lb_read(int registerAndIndex);

unsigned char lbu_read(int registerAndIndex);
// WriteBack Stage helpers headers

int lw_write(int register1, int value);

short lh_write(short register1, int value);

unsigned short lhu(int register2, int value);

char lb(int register1, int value);

unsigned char lbu(int register1, int value);

void sw(int register1, int registerAndIndex);

void sh(int register1, int registerAndIndex);

void sb(int register1, int registerAndIndex);

void mfhi(int register2);

void mflo(int register2);

void mtcZ(int register1, int register2);



/*america() frees all memory that has been malloc'ed and not otherwise freed
IF YOU HAVE CALLED malloc() MAKE SURE TO FREE THAT MEMORY. DO IT HERE IF THERE
IS NO BETTER PLACE. Thanks.
Current memory that is deallocated here:
instructions: the array holding all instructions in char* form
jump_names: the array holding all jump names in char* form
*/
void america();

int main(int argc, char *argv[]) {
    int i=0; char *a;

	for (i = 1; i< argc; i++) {
		printf("\narg%d=%s", i, argv[i]);
		if (i = 1) {
			char *filenm = argv[i]; //Input file
		}
	}

    readmips(filenm);     
    /*
    while(instructions[i]!=NULL){
        printf("%s\n",instructions[i]);
        ++i;
    }
    i=0;
    while(jump_names[i]!=NULL){
        printf("%d, %s\n", jump_locations[i], jump_names[i]);
        if((instructions[jump_locations[i]]!=NULL)&&(strcmp(instructions[jump_locations[i]],"")!=0)){
            printf("%s\n", instructions[jump_locations[i]]);
        } else{
            printf("WOWOOWOOWOWOOW\n");
        }
        i++;
    }*/
    printf("\n\n\n\n\n\n");
    controllogic();
    
    america();
    return 0;
}

void readmips(char* filename){
    FILE * assembly = fopen(filename, "r");
    int lineno=0; int charno=0; int i=0; int spacebreak=0; int jumpno=0;
    char buffer[128];
    char* temp; char* temploc;
    int abc=0;
    if (assembly == 0)
    {
        //fopen returns 0, the NULL pointer, on failure
        perror("Cannot open input file\n");
        exit(-1);
    }
    else {
        while(fgets(buffer,128,assembly)!=NULL){
            charno=0;
            while(buffer[charno]!='#' && buffer[charno]!='\n'){
                charno++;
            } 
            temp = (char*)malloc(sizeof(char)*(charno+1));
            memset(temp,0,charno+1); // clear new memory
            i=0; spacebreak=0;
            while(isspace(buffer[i])){
                i++;
            }
            spacebreak=i;
            while(i<charno){
                if(buffer[i]==':'){ // jump location parse
                    temploc=(char*)malloc(i+1);
                    memset(temploc,0,i+1);
                    strcpy(temploc, temp);
                    jump_names[jumpno]=temploc; // save jump name
                    jump_locations[jumpno]=lineno; // and the instruction name
                    jumpno++;
                    memset(temp,0,i+1); //reset the memory
                    while(isspace(buffer[++i])){}
                    spacebreak=i;
                }
                *(temp+i-spacebreak)=buffer[i];
                i++;
            }
            
            if (charno!=0 && spacebreak<charno){
                instructions[lineno]=temp;
                lineno++; //rewrite over memory if the line has no instruction
            }
            abc++;
        }
    }
}

void printstate(){
    int i;    
    for(i=0;i<32;i++){
        printf("%s:%d,",RegisterFileNames[i],RegisterFile[i]);
    } printf("\n");
    for(i=0;i<70;i++){
        printf("%d,",data_memory[i]);
    } printf("\n");
}

int controllogic(){
    int pc=0;
    char *cur_instruction; //analogous to if/id pipeline register
    struct j_type *j_instr_ptr;
    struct i_type *i_instr_ptr;
    struct r_type *r_instr_ptr;
    struct Instruction_Type* instr;
    struct exmem* exmem_ptr;
    struct memwb* memwb_ptr;
    int mainct =0;
    printf("Control initiated:\n");

	pthread_t *threads;
    while(1){
		pthread_create(&threads[0], NULL, getinstructions, (void*) &pc);
		pthread_create(&threads[1], NULL, decodeinstruction, (void*) cur_instruction);
		pthread_create(&threads[2], NULL, execute, (void*) instr);
		pthread_create(&threads[3], NULL, memory_rw, (void*) exmem_ptr);
		pthread_create(&threads[4], NULL, writeback, (void*) memwb_ptr);

        printf("pc:%d\n",pc);
        printstate();
        cur_instruction = getinstruction(pc++); //instruction fetch is also tasked with incrementing pc
        printf("%s, %d\n", cur_instruction,(int)strlen(cur_instruction));
        instr = decodeinstruction(cur_instruction);
        if((*instr).instr==Break){
            break;
        }
        exmem_ptr = execute(instr, &pc);
        memwb_ptr = memory_rw(exmem_ptr);
        writeback(memwb_ptr);

		// Wait for each thread to finish
		pthread_join(threads[0], NULL);
		pthread_join(threads[1], NULL);
		pthread_join(threads[2], NULL);
		pthread_join(threads[3], NULL);
		pthread_join(threads[4], NULL);

		// move data from intermediate to used args


    }
    return 0;
}

void* fetchvoid(void* void_pc){
    return (void*)getinstruction((int*)void_pc);
}

char* getinstruction(int *pc_ptr){
    int pc = *pc_ptr;
    char* instr;
    if(instructions[pc]!=NULL){
        instr = instructions[pc];
    } else{
        instr = "";
    }
    pc++; *pc_ptr = pc;
    return instr;
}

void* decodevoid(void* void_instr){
    return (void*)decodeinstruction((char*)void_instr);
}

struct Instruction_Type* decodeinstruction(char* instr){
    char buffer[8];
    int i=0; int instrnum; int* ptr = &i;
    struct j_type *j_instr_ptr;
    struct i_type *i_instr_ptr;
    struct r_type *r_instr_ptr;
    struct Instruction_Type instr_type;
    struct Instruction_Type* instr_ptr=&instr_type;
    memset(instr_ptr,0,sizeof(instr_type));
    memset(buffer,0,sizeof(buffer));

    if(strlen(instr)==0){
        instrnum=Break;
    } else{
        while(!isspace(*(instr+i))){
            buffer[i]=*(instr+i);
            i++;
        }
        for(instrnum=0;instrnum<44;instrnum++){
            if(strcmp(buffer,instr_name_list[instrnum])==0){
                break;
            }
        }
    }
    if(instrnum<Addi){
        r_instr_ptr=make_r_type(instrnum, instr, ptr);
        printf("inum:%d,dest:%d,s1:%d,s2:%d,shamt:%d\n",(*r_instr_ptr).instr,(*r_instr_ptr).dest_register,(*r_instr_ptr).s_register1,(*r_instr_ptr).s_register2,(*r_instr_ptr).shamt);
        instr_ptr=(struct Instruction_Type*)r_instr_ptr;
    } else if(instrnum<J){
        i_instr_ptr=make_i_type(instrnum, instr, ptr);
        printf("inum:%d,dest:%d,s:%d,imm:%d\n",(*i_instr_ptr).instr,(*i_instr_ptr).dest_register,(*i_instr_ptr).s_register,(*i_instr_ptr).immediate);
        instr_ptr=(struct Instruction_Type*)i_instr_ptr;
    } else if (instrnum<Break){
        j_instr_ptr=make_j_type(instrnum, instr, ptr);
        printf("inum:%d,jump:%d\n",(*j_instr_ptr).instr,(*j_instr_ptr).jump_to);
        instr_ptr=(struct Instruction_Type*)j_instr_ptr;
    } else if (instrnum==Break){
        instr_type.instr=Break;
    } else{
        instr_type.instr=Break+1;
        printf("Instruction not found\n");
    }
    
    return instr_ptr;
}

void* executevoid(void* instr_type_void){
    return (void*)execute((struct Instruction_Type*)instr_type_void);
}

struct exmem* execute(struct Instruction_Type* instr){
    struct j_type *j_instr_ptr;
    struct i_type *i_instr_ptr;
    struct r_type *r_instr_ptr;
    struct exmem *exmem_ptr;
    int new_pc = *pc_ptr;
    
    if((*instr).instr<Addi){
        r_instr_ptr=(struct r_type*)instr;
        exmem_ptr = execute_r(r_instr_ptr);
    } else if((*instr).instr<J){
        i_instr_ptr=(struct i_type*)instr;
        exmem_ptr = execute_i(i_instr_ptr);
    } else if((*instr).instr<Break){
        j_instr_ptr=(struct j_type*)instr;
        exmem_ptr = execute_j(j_instr_ptr,new_pc);
    } else{
        exit(1);
    }
    
    if((*exmem_ptr).will_branch!=0){
        new_pc = (*exmem_ptr).will_branch;
        *pc_ptr = new_pc;
    }

    return exmem_ptr;
}

struct exmem* execute_r(struct r_type *r_type_ptr){
    struct r_type r_instr = *r_type_ptr;
    struct exmem* exmem_ptr = (struct exmem*)calloc(1,sizeof(struct exmem));
    struct exmem exmem_reg = *exmem_ptr;

    (*exmem_ptr).instrnum=r_instr.instr; (*exmem_ptr).will_branch=0;
    if(r_instr.instr==Jr) { // get address from RF
        (*exmem_ptr).will_branch = r_instr.dest_register;
    } else if(r_instr.instr>=Mfhi) { //mfhi, mflo save to register
        (*exmem_ptr).dest_register = r_instr.dest_register;
    } else if(r_instr.instr==MfcZ){ // save to in dest, save from location in s_r1
        (*exmem_ptr).dest_register = r_instr.dest_register;
        (*exmem_ptr).alu_result = r_instr.s_register1;
    } else if(r_instr.instr==MtcZ){ // save value in dest, save to location in sr1
        (*exmem_ptr).dest_register = r_instr.s_register1; // location for cZ
        (*exmem_ptr).alu_result = r_instr.dest_register; // value from reg
    } else if(r_instr.instr>=Mult){ // all mult, div operations, save values to 2 source registers
        alu(r_instr.s_register1, r_instr.s_register2, r_instr.instr);
        (*exmem_ptr).alu_result = 0;
    } else if(r_instr.instr>=Sll){ // shift operations - dest, value of s_r, shamt
        (*exmem_ptr).dest_register = r_instr.dest_register;
        (*exmem_ptr).alu_result = alu(r_instr.s_register1, r_instr.shamt, r_instr.instr);
    } else{ //all normal 3 register operations - values of 2 source regs
        (*exmem_ptr).dest_register = r_instr.dest_register;
        (*exmem_ptr).alu_result = alu(r_instr.s_register1, r_instr.s_register2, r_instr.instr);
    }
    free(r_type_ptr);
    
    printf("instr:%d,alu:%d,dest:%d,branch:%d\n",(*exmem_ptr).instrnum,(*exmem_ptr).alu_result,(*exmem_ptr).dest_register,(*exmem_ptr).will_branch);
    return exmem_ptr;
}

struct exmem* execute_i(struct i_type *i_type_ptr){
    struct i_type i_instr = *i_type_ptr;
    struct exmem *exmem_ptr = (struct exmem*)calloc(1,sizeof(struct exmem));
    struct exmem exmem_reg = *exmem_ptr;

    (*exmem_ptr).instrnum=i_instr.instr; (*exmem_ptr).will_branch=0;
    if(i_instr.instr==Lui){ // lui needs dest and imm
        (*exmem_ptr).dest_register = i_instr.dest_register;
        (*exmem_ptr).alu_result=i_instr.immediate << 16;
    } else if(i_instr.instr>=Sw){ // stores need value of dest, s, imm
        (*exmem_ptr).dest_register = i_instr.dest_register;
        (*exmem_ptr).alu_result = alu(i_instr.s_register, i_instr.immediate, Addi);
    } else if(i_instr.instr>=Lw){ //writes to dest_reg, needs value of s, imm
        (*exmem_ptr).dest_register = i_instr.dest_register;
        (*exmem_ptr).alu_result = alu(i_instr.s_register, i_instr.immediate, Addi);
    } else if(i_instr.instr==Bne){
        if((alu(i_instr.s_register, i_instr.dest_register,Slt)==1)||(alu(i_instr.dest_register, i_instr.s_register,Slt)==1)){
            (*exmem_ptr).will_branch=i_instr.immediate;
        }
    } else if(i_instr.instr==Beq){
        if((alu(i_instr.s_register, i_instr.dest_register,Slt)==0)&&(alu(i_instr.dest_register, i_instr.s_register,Slt)==0)){
            (*exmem_ptr).will_branch=i_instr.immediate;
        }
    } else{
        (*exmem_ptr).alu_result = alu(i_instr.s_register, i_instr.immediate, i_instr.instr);
        (*exmem_ptr).dest_register = i_instr.dest_register;
    }
    free(i_type_ptr);
    printf("instr:%d,alu:%d,dest:%d,branch:%d\n",(*exmem_ptr).instrnum,(*exmem_ptr).alu_result,(*exmem_ptr).dest_register,(*exmem_ptr).will_branch);
    return exmem_ptr;
}

struct exmem* execute_j(struct j_type *j_type_ptr, int pc){
    struct j_type j_instr = *j_type_ptr;
    struct exmem* exmem_ptr = (struct exmem*)calloc(1,sizeof(struct exmem));
    struct exmem exmem_reg = *exmem_ptr;
    
    (*exmem_ptr).instrnum=j_instr.instr;
    if(j_instr.instr==J){ // j
        (*exmem_ptr).will_branch=j_instr.jump_to;
    } else{ // jal
        (*exmem_ptr).will_branch=j_instr.jump_to;
        (*exmem_ptr).alu_result=pc; (*exmem_ptr).dest_register=31;
    }
    free(j_type_ptr);
    
    printf("instr:%d,alu:%d,dest:%d,branch:%d\n",(*exmem_ptr).instrnum,(*exmem_ptr).alu_result,(*exmem_ptr).dest_register,(*exmem_ptr).will_branch);
    return exmem_ptr;
}


//ALU function

int alu(int operandA, int operandB, int Operation) {
	int result;
	if (Operation == Add) {
		result = add(operandA, operandB);
	}
	else if (Operation == Addi) {
		result = addi(operandA, operandB);
	}
	else if (Operation == Addu) {
		result = addu(operandA, operandB);
	}
	else if (Operation == Addiu) {
		result = addiu(operandA, operandB);
	}
	else if (Operation == Sub) {
		result = sub(operandA, operandB);
	}
	else if (Operation == Subu) {
		result = subu(operandA, operandB);
	}
	else if (Operation == Mult) {
		mult(operandA, operandB);
	}
	else if (Operation == Multu) {
		multu(operandA, operandB);
	}
	else if (Operation == MDiv) {
		mdiv(operandA, operandB);
	}
	else if (Operation == Divu) {
		divu(operandA, operandB);
	}
	else if (Operation == And) {
		result = and(operandA, operandB);
	}
	else if (Operation == Andi) {
		result = andi(operandA, operandB);
	}
	else if (Operation == Or) {
		result = or(operandA, operandB);
	}
	else if (Operation == Ori) {
		result = ori(operandA, operandB);
	}
	else if (Operation == Xor) {
		result = xor (operandA, operandB);
	}
	else if (Operation == Nor) {
		result = nor(operandA, operandB);
	}
	else if (Operation == Slt) {
		result = slt(operandA, operandB);
	}
	else if (Operation == Slti) {
		result = slti(operandA, operandB);
	}
	else if (Operation == Sltu) {
		result = sltu(operandA, operandB);
	}
	else if (Operation == Sll) {
		result = sll(operandA, operandB);
	}
	else if (Operation == Sllv) {
		result = sllv(operandA, operandB);
	}
	else if (Operation == Srl) {
		result = srl(operandA, operandB);
	}
	else if (Operation == Srlv) {
		result = srlv(operandA, operandB);
	}
	else if (Operation == Sra) {
		result = sra(operandA, operandB);
	}
	else if (Operation == Srav) {
		result = srav(operandA, operandB);
	}
	else {
		perror("Command Not Found\n");
		exit(-1);
	}
	return result;
}

void* memory_rw_void(void* exmem_void){
    return (void*)memory_rw((struct exmem*) exmem_void);
}


struct memwb *memory_rw(struct exmem* exmem_ptr){
    struct exmem exmem_reg = *exmem_ptr;
    struct memwb *memwb_ptr = (struct memwb*)calloc(1, sizeof(struct memwb));
    struct memwb memwb_reg = *memwb_ptr;

    (*memwb_ptr).instrnum=exmem_reg.instrnum;
    (*memwb_ptr).dest_register=exmem_reg.dest_register;
    if(exmem_reg.instrnum==Lw){
        (*memwb_ptr).value=lw_read(exmem_reg.alu_result);
    } else if(exmem_reg.instrnum==Lh){
        (*memwb_ptr).value=(int)lh_read(exmem_reg.alu_result);
    } else if(exmem_reg.instrnum==Lhu){
        (*memwb_ptr).value=(int)lhu_read(exmem_reg.alu_result);
    } else if(exmem_reg.instrnum==Lb){
        (*memwb_ptr).value=(int)lb_read(exmem_reg.alu_result);
    } else if(exmem_reg.instrnum==Lbu){
        (*memwb_ptr).value=(int)lbu_read(exmem_reg.alu_result);
    } else if(exmem_reg.instrnum==Sw){
        sw(exmem_reg.dest_register,exmem_reg.alu_result);
        (*memwb_ptr).dest_register=0;
    } else if(exmem_reg.instrnum==Sh){
        sh(exmem_reg.dest_register,exmem_reg.alu_result);
        (*memwb_ptr).dest_register=0;
    } else if(exmem_reg.instrnum==Sb){
        sb(exmem_reg.dest_register,exmem_reg.alu_result);
        (*memwb_ptr).dest_register=0;
    } else{
        (*memwb_ptr).value=exmem_reg.alu_result;
    }
    free(exmem_ptr);
    printf("instr:%d,dest:%d,val:%d\n",memwb_reg.instrnum,memwb_reg.dest_register,memwb_reg.value);
    return memwb_ptr;
}

void* writeback(void* memwb_void){
    writeback((struct memwb*) memwb_void);
    return NULL;
}

void writeback(struct memwb* memwb_ptr){
    struct memwb memwb_reg = *memwb_ptr;
    
    printf("instr:%d,dest:%d,val:%d\n",memwb_reg.instrnum,memwb_reg.dest_register,memwb_reg.value);
    if(memwb_reg.instrnum==Mfhi){
        mfhi(memwb_reg.dest_register);
    } else if(memwb_reg.instrnum==Mflo){
        mflo(memwb_reg.dest_register);
    } else if(memwb_reg.instrnum==MtcZ){
        mtcZ(memwb_reg.dest_register,memwb_reg.value);
    } else{
        register_write(memwb_reg.dest_register,memwb_reg.value);
    }
    
    free(memwb_ptr);
}



// decode helper functions
struct r_type* make_r_type(int instrnum, char* instr, int* ptr){
    struct r_type *r_type_ptr=(struct r_type*)calloc(1, sizeof(struct r_type*));
    struct r_type r_instr = *r_type_ptr;

    (*r_type_ptr).instr=instrnum; (*r_type_ptr).pc_ptr = NULL;
    if(instrnum==Jr) { // get address from RF
        (*r_type_ptr).dest_register=RegisterFile[nextregister(instr, ptr)];
    } else if(instrnum>=Mfhi) { //mfhi, mflo save to register
        (*r_type_ptr).dest_register=nextregister(instr, ptr);
    } else if(instrnum==MfcZ){ // save to in dest, save from location in s_r1
        (*r_type_ptr).dest_register=nextregister(instr, ptr);
        (*r_type_ptr).s_register1=controlRegisterZ[RegisterFile[nextregister(instr, ptr)]];
    } else if(instrnum==MtcZ){ // save value in dest, save to location in sr1
        (*r_type_ptr).dest_register=RegisterFile[nextregister(instr, ptr)]; //value
        (*r_type_ptr).s_register1=RegisterFile[nextregister(instr, ptr)]; // location
    } else if(instrnum>=Mult){ // all mult, div operations, save values to 2 source registers
        (*r_type_ptr).s_register1=RegisterFile[nextregister(instr, ptr)];
        (*r_type_ptr).s_register2=RegisterFile[nextregister(instr, ptr)];
    } else if(instrnum>=Sll){ // shift operations - dest, value of s_r, shamt
        (*r_type_ptr).dest_register=nextregister(instr, ptr);
        (*r_type_ptr).s_register1=RegisterFile[nextregister(instr, ptr)];
        (*r_type_ptr).shamt=nextint(instr, ptr);
    } else{ //all normal 3 register operations - values of 2 source regs
        (*r_type_ptr).dest_register=nextregister(instr, ptr);
        (*r_type_ptr).s_register1=RegisterFile[nextregister(instr, ptr)];
        (*r_type_ptr).s_register2=RegisterFile[nextregister(instr, ptr)];
    }
    printf("instr:%d,dest:%d,s1:%d,s2:%d\n",(*r_type_ptr).instr,(*r_type_ptr).dest_register,(*r_type_ptr).s_register1,(*r_type_ptr).s_register2);
    return r_type_ptr;
}

struct i_type* make_i_type(int instrnum, char* instr, int* ptr){
    struct i_type *i_type_ptr=(struct i_type*)calloc(1, sizeof(struct i_type*));
    struct i_type i_instr = *i_type_ptr;
    struct indexed_register *indreg_ptr;

    (*i_type_ptr).instr=instrnum; (*i_type_ptr).pc_ptr = NULL;
    
    if(instrnum==Lui){ // lui needs dest and imm
        (*i_type_ptr).dest_register=nextregister(instr, ptr);
        (*i_type_ptr).immediate=nextint(instr, ptr);
    } else if(instrnum>=Sw){ // stores need value of dest, s, imm
        (*i_type_ptr).dest_register=RegisterFile[nextregister(instr, ptr)];
        indreg_ptr = nextindexedregister(instr, ptr);
        (*i_type_ptr).s_register=RegisterFile[(*indreg_ptr).mem_register]; (*i_type_ptr).immediate=(*indreg_ptr).index;
        free(indreg_ptr);
    } else if(instrnum>=Lw){ //writes to dest_reg, needs value of s, imm
        (*i_type_ptr).dest_register=nextregister(instr, ptr);
        indreg_ptr = nextindexedregister(instr, ptr);
        (*i_type_ptr).s_register=RegisterFile[(*indreg_ptr).mem_register]; (*i_type_ptr).immediate=(*indreg_ptr).index;
        free(indreg_ptr);
    } else if(instrnum>=Beq){
        (*i_type_ptr).dest_register=RegisterFile[nextregister(instr, ptr)];
        (*i_type_ptr).s_register=RegisterFile[nextregister(instr, ptr)]; (*i_type_ptr).immediate=nextjumploc(instr,ptr);
    } else{
        (*i_type_ptr).dest_register=nextregister(instr, ptr);
        (*i_type_ptr).s_register=RegisterFile[nextregister(instr, ptr)]; (*i_type_ptr).immediate=nextint(instr, ptr);
    }
    printf("instr:%d,dest:%d,s:%d,imm:%d\n",(*i_type_ptr).instr,(*i_type_ptr).dest_register,(*i_type_ptr).s_register,(*i_type_ptr).immediate);
    return i_type_ptr;
}

struct j_type* make_j_type(int instrnum, char* instr, int* ptr){
    struct j_type *j_type_ptr=(struct j_type*)calloc(1, sizeof(struct j_type*));
    struct j_type j_instr = *j_type_ptr;

    (*j_type_ptr).instr=instrnum; (*j_type_ptr).pc_ptr = NULL;
    (*j_type_ptr).jump_to=nextjumploc(instr, ptr);
    
    printf("instr:%d,jump:%d\n",(*j_type_ptr).instr,(*j_type_ptr).jump_to);
    return j_type_ptr;
}

// second level decode helper functions
char nextregister(char* instr, int* ptr){
    char reg[8];
    int i=*ptr; char newit = 0;
    memset(reg,0,sizeof(reg));
    
    while(isspace(*(instr+i))){
        i++;
    }
    while(!isspace(*(instr+i)) && *(instr+i)!=',' && *(instr+i)!=0){
        reg[newit]=*(instr+i);
        i++;
        newit++;
    }
    i++; *ptr=i;
    
    for(newit=0;newit<32;newit++){
        if(strcmp(reg,RegisterFileNames[newit])==0||strcmp(reg,RegisterNumberNames[newit])==0){
            return newit;
        }
    }
    return 32;
}

int nextint(char* instr, int* ptr){
    int i=*ptr;
    while(isspace(*(instr+i))){
        i++;
    }
    return (int) strtol(instr+i,(char**)NULL,10);
}

int nextjumploc(char* instr, int* ptr){
    char jump[32]; int newit=0; int i=*ptr;
    memset(jump,0,sizeof(jump));
    
    while(isspace(*(instr+i))){i++;}
    while(!isspace(*(instr+i)) && *(instr+i)!=0){
        jump[newit]=*(instr+i);
        i++; newit++;
    }   
    
    newit=0; // redefine to check if a jump location was found
    i=0; // redefine as iterator through jump_locations
    while(jump_names[i]!=NULL){
        if(strcmp(jump_names[i],jump)==0){
            newit=1;
            break;
        }
        i++;
    }
    if(newit!=1){
        printf("Jump location not found: %s\n", jump);
        return 0;
    }
    return jump_locations[i];
}

struct indexed_register* nextindexedregister(char* instr, int* ptr){
    char reg[8];
    int i=*ptr; int start; char newit;
    struct indexed_register *indreg_ptr=(struct indexed_register*)calloc(1,sizeof(struct indexed_register));
    struct indexed_register indreg = *indreg_ptr;
    
    while(isspace(*(instr+i))){i++;}
    start=i;
    (*indreg_ptr).index = strtol(instr+start,(char**)NULL,10);
    while(*(instr+i)!='('){i++;}
    i++;
    start=i;
    while(!isspace(*(instr+i)) && *(instr+i)!=')'){
        reg[i-start]=*(instr+i);
        i++;
    }
    i = 0;
    for(newit=0;newit<32;newit++){
        if(strcmp(reg,RegisterFileNames[newit])==0||strcmp(reg,RegisterNumberNames[newit])==0){
            (*indreg_ptr).mem_register=newit;
            i=1;
            break;
        }
    }
    if(i!=1){(*indreg_ptr).mem_register=32;}
    return indreg_ptr;
}

// Check for overflow function
int safe_add(int a, int b) {
	if (a > 0 && b > INT_MAX - a) {
		perror("Overflow Error");
		exit(-1);
	}
	else if (a < 0 && b < INT_MIN - a) {
		perror("Overflow Error");
		exit(-1);
	}
	return a + b;
}

//Go from signed to two'c complement
int sm2tc(int x) {
	int m = x >> 31;
	return (~m & x) | (((x & 0x80000000) - x) & m);
}

//Calculate the necessary jump for Srav and others

int summing(int x) {
	int count = 1;
	int sum = 0;
	while (count < x) {
		sum += (1 << (32 - count));
	}
	return sum;
}

// Start of Arithmetic functions
// Mult and Divide, all operations with 'u' have to use unsigned values, others have to check for overflow
int add(int register2, int register3) {
	return safe_add(register2, register3);
}

unsigned int addu(int register2, int register3) {
	return register2 + register3;
}

int sub(int register2, int register3) {
	return safe_add(register2, 0-register3);
}

unsigned int subu(int register2, int register3) {
	return register2 - register3;
}

int addi(int register2, int number) {
	return safe_add(register2, number);
}

unsigned int addiu(int register2, int number) {
	return register2 + number;
}

void mult(int register2, int register3) {
	long long unsigned int mult1;
	long long unsigned int mult2;
	mult1 = (long long int) register2;
	mult2 = (long long int) register3;
	LO = ((mult1 * mult2) << 32) >> 32;
    printf("lo:%d\n",LO);
	HI = ( mult1* mult2) >> 32;
    printf("hi:%d\n",HI);
}

void multu(int register2, int register3) {
	long long unsigned int mult1;
	long long unsigned int mult2;
	mult1 = (long long unsigned int) register2;
	mult2 = (long long unsigned int) register3;
	LO = ((mult1 * mult2) << 32) >> 32;
	HI = (mult1* mult2) >> 32;
}

void mdiv(int register2, int register3) {
	LO = register2 / register3;
	HI = register2 % register3;
}

void divu(int register2, int register3) {
	LO = register2 / register3;
	HI = register2 % register3;
}

//Start Bitwise Operation Functions

int sll(int register2, int number) {
	return register2 << number;
}

int srl(int register2, int number) {
	return register2 >> number;
}

int sllv(int register2, int register3) {
	return register2 << register3;
}

int srlv(int register2, int register3) {
	return register2 >> register3;
}

int sra(int register2, int number) {
	return (register2 >> number + summing(number) * (register2 >> 31));
}

int srav(int register2, int register3) {
	return (register2 >> register3 + summing(register3) * (register2 >> 31));
}

// Start Data Transfer functions

// Memory Stage helpers

// Read in Load Word Memory (Memory Stage)
int lw_read(int registerAndIndex) {
	return data_memory[registerAndIndex/4];
}

// Read in Load Halfword Memory (Memory Stage)
short lh_read(int registerAndIndex) {
	int a = data_memory[registerAndIndex/4];
	short b = (short)a; //TODO: make this get the right half
	// Need to figure out which half to get (distinguish from each other)
	return b;
}

unsigned short lhu_read(int registerAndIndex){
    int a = data_memory[registerAndIndex/4];
    unsigned short b = (unsigned short)a; //TODO: make this get the right half
}


// Read in Byte Memory (Memory Stage)
char lb_read(int registerAndIndex) {
	int a = data_memory[registerAndIndex/4];
	char b = (char)a; //TODO: make this get the right byte
	return b;
}

unsigned char lbu_read(int registerAndIndex){
    int a = data_memory[registerAndIndex/4];
    unsigned char b = (unsigned char)a; //TODO: make this get the right byte
    return b;
}


// WriteBack Stage helpers

// Load Word (WriteBack Stage)
void register_write(int register1, int value) {
	if (register1 != 0){
		RegisterFile[register1] = value;
	} 
}

// Load Halfword (WriteBack Stage)
short lh_write(short register1, int value) {
	if (value % 4 == 0 || value % 4 == 1) {
		register1 = (data_memory[value/4] >> 16);
	} else if (value % 4 == 2 || value % 4 == 3) {
		register1 = ((data_memory[value/4] << 16) >> 16);
	}
}


// Load Halfword Unsigned (WriteBack Stage)
unsigned short lhu(int register1, int value) {
	if (value % 4 == 0 || value % 4 == 1) {
		register1 = (data_memory[value/4] >> 16);
	} else if (value % 4 == 2 || value % 4 == 3) {
		register1 = ((data_memory[value/4] << 16) >> 16);
	}
}


// Load Byte (WriteBack Stage)
char lb(int register1, int value) {
	if (value % 4 == 0) {
		register1 = data_memory[value/4] >> 24;
	} else if (value % 4 == 1) {
		register1 = (data_memory[value/4] << 8) >> 24;
	} else if (value % 4 == 2) {
		register1 = (data_memory[value/4] << 16) >> 24;
	} else if (value % 4 == 3) {
		register1 = (data_memory[value/4] << 24) >> 24;
	}
}

// Load Byte Unsigned (WriteBack Stage)
unsigned char lbu(int register1, int value) {
	if (value % 4 == 0) {
		register1 = data_memory[value/4] >> 24;
	} else if (value % 4 == 1) {
		register1 = (data_memory[value/4] << 8) >> 24;
	} else if (value % 4 == 2) {
		register1 = (data_memory[value/4] << 16) >> 24;
	} else if (value % 4 == 3) {
		register1 = (data_memory[value/4] << 24) >> 24;
	}
}

// Store Word (WriteBack Stage)
void sw(int register1, int registerAndIndex) {
	data_memory[registerAndIndex/4] = register1;
}


// Store Halfword (WriteBack Stage)
void sh(int register1, int registerAndIndex) {
	if (registerAndIndex % 4 == 2 || registerAndIndex % 4 == 3) {
		data_memory[registerAndIndex/4] = ((data_memory[registerAndIndex/4] >> 16) << 16) + register1;
	} else if (registerAndIndex % 4 == 0 || registerAndIndex % 4 == 1) {
		data_memory[registerAndIndex/4] = ((data_memory[registerAndIndex/4] << 16) >> 16) + (register1 << 16);
	}
}

// Store Byte (WriteBack Stage)
void sb(int register1, int registerAndIndex) {
	if (registerAndIndex % 4 == 0) {
		data_memory[registerAndIndex/4] = ((data_memory[registerAndIndex/4] << 8) >> 16) + (register1 << 24);
	} else if (registerAndIndex % 4 == 1) {
		data_memory[registerAndIndex/4] = ((data_memory[registerAndIndex/4] << 16) >> 16) + ((data_memory[registerAndIndex/4] >> 24) << 24) + (register1 << 16);
	} else if (registerAndIndex % 4 == 2) {
		data_memory[registerAndIndex/4] = ((data_memory[registerAndIndex/4] << 24) >> 24) + ((data_memory[registerAndIndex/4] >> 16) << 16) + (register1 << 8);
	} else if (registerAndIndex % 4 == 3) {
		data_memory[registerAndIndex/4] = ((data_memory[registerAndIndex/4] >> 8) << 8) + register1;
	}
}


// Move from HI (WriteBack Stage)
void mfhi(int register2) {
	register2= HI;
}

// Move from LO (WriteBack Stage)
void mflo(int register2) {
	register2 = LO;
}

void mtcZ(int Zregister, int value){
    controlRegisterZ[Zregister]=value;
}



// Start of Logical functions

int and(int register2, int register3) {
	return (register2 & register3);
}

int andi(int register2, int number) {
	return (register2 & number);
}

int or(register2, register3) {
    return (register2 | register3);
}

int ori(int register2, int number) {
	return (register2 | number);
}

int xor(int register2, int register3) {
	return (register2 ^ register3);
}

int nor(int register2, int register3) {
	return (~(register2 | register3));
}

int slt(int register2, int register3) {
	if (register2 < register3) {
		return 1;
	}
	else {
		return 0;
	}
}

unsigned int sltu(int register2, int register3) {
	if (register2 < register3) {
		return 1;
	}
	else {
		return 0;
	}
}

int slti(int register2, int number) {
	if (register2 < number) {
		return 1;
	}
	else {
		return 0;
	}
}

/*
________$ 
_______$$ 
___$$$$$$ 
_$$_O-$$$ 
$$$$$$$$$$ 
_$$__$$$$$ 
_$___$$$$    ~  ~   HUMP DAY!!!   ~  ~
_$___$$$$ 
_$__$$$$ 
_$__$$$$ 
_$_$$$$$_______$$$$$ 
_$_$$$$_______$$$$$$$ 
_$_$$$$$$____$$$$$$$$$ 
_$__$$$$$$$$$$$$$$$$$$$$ 
__$_$$$$$$$$$$$$$$$$$$$$$ 
___$$$$$$$$$$$$$$$$$$$$$$$ 
_____$$$$$$$$$$$$$$$$$$$$$ 
______$$$$$$$$$$$$$$$$$$$ 
______$$$$$$______$$$$$$$ 
_______$$$$_______$$$$$$ 
_______$$$_________$$$$$ 
_______$$$_________$$$_$ 
_______$$$_________$$$_$ 
_______$$$__________$$_$ 
_______$$$__________$$$$ 
______$$___________$$$

*/

/*
________________________________________________________________
|  |*   *   *   *   *   *|--------------------------------------|
|  |  *   *   *   *   *  |                                      |
|  |*   *   *   *   *   *|--------------------------------------|
|  |  *   *   *   *   *  |                                      |
|  |*   *   *   *   *   *|--------------------------------------|
|  |  *   *   *   *   *  |                                      |
|  |*   *   *   *   *   *|--------------------------------------|
|  |  *   *   *   *   *  |                                      |
|  |*   *   *   *   *   *|--------------------------------------|
|  |---------------------                                       |
|  |                                                            |
|  |------------------------------------------------------------|
|  |                                                            |
|  |------------------------------------------------------------|
|  |                                                            |
|  |-------------------------------------------------------------
|  |
|  |
|  |   
|  |
|  |
|  | 
|  |
|  |
|  | 
|  |                   ~   'MERICA.   ~
|  |
|  | 
|  |
|  |
|  | 
|  |
|  |
|  | 
|  |
|  |
|  | 
|  |
|  |
|  | 

*/



// free everything you've malloc'ed
void america(){
    int i = 0;
    // instructions were malloc'ed
    while(instructions[i]!= NULL){
        free(instructions[i++]);
    } 
    i=0;
    // jump_names were malloc'ed
    while(jump_names[i]!= NULL){
        free(jump_names[i++]);
    }
}


/*                                  ~   It's time for some pipelining boys.   ~
                                            

___________________▄▄▄▀▀▀▀▀▀▀▄
 _______________▄▀▀____▀▀▀▀▄____█
 ___________▄▀▀__▀▀▀▀▀▀▄___▀▄___█
 __________█▄▄▄▄▄▄_______▀▄__▀▄__█
 _________█_________▀▄______█____█_█
 ______▄█_____________▀▄_____▐___▐_▌
 ______██_______________▀▄___▐_▄▀▀▀▄
 ______█________██_______▌__▐▄▀______█                    ~   HERE WE GO!   ~
 ______█_________█_______▌__▐▐________▐
 _____▐__________▌_____▄▀▀▀__▌_______▐_____________▄▄▄▄▄▄
 ______▌__________▀▀▀▀________▀▀▄▄▄▀______▄▄████▓▓▓▓▓▓▓███▄
 ______▌____________________________▄▀__▄▄█▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▄
 ______▐__________________________▄▀_▄█▓▓▓▓▓▓▓▓▓▓_____▓▓____▓▓█▄
 _______▌______________________▄▀_▄█▓▓▓▓▓▓▓▓▓▓▓____▓▓_▓▓_▓▓__▓▓█
 _____▄▀▄_________________▄▀▀▌██▓▓▓▓▓▓▓▓▓▓▓▓▓__▓▓▓___▓▓_▓▓__▓▓█
 ____▌____▀▀▀▄▄▄▄▄▄▄▄▀▀___▌█▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓__▓________▓▓___▓▓▓█
 _____▀▄_________________▄▀▀▓▓▓▓▓▓▓▓█████████████▄▄_____▓▓__▓▓▓█
 _______█▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█▓▓▓▓▓██▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓██▄▄___▓▓▓▓▓█
 _______█▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█▓▓███▓▓▓▓████▓▓▓▓▓▓▓▓▓▓▓▓▓██▓▓▓▓▓▓█
 ________█▓▓▓▓▓▓▓▓▓▓▓▓▓▓█▓█▓▓██░░███████░██▓▓▓▓▓▓▓▓▓▓██▓▓▓▓▓█
 ________█▓▓▓▓▓▓▓▓▓▓▓▓▓▓██▓░░░░░█░░░░░██░░░░██▓▓▓▓▓▓▓▓▓██▓▓▓▓▌
 ________█▓▓▓▓▓▓▓▓▓▓▓▓▓▓███░░░░░░░░____░██░░░░░░░██▓▓▓▓▓▓▓██▓▓▌
 ________▐▓▓▓▓▓▓▓▓▓▓▓▓▓▓██░░░░░░░________░░░░░░░░░██████▓▓▓▓▓█▓▌
 ________▐▓▓▓▓▓▓▓▓▓▓▓▓▓▓██░░░░░░___▓▓▓▓▓░░░░░░░███░░███▓▓▓▓▓█▓▌
 _________█▓▓▓▓▓▓▓▓▓▓▓▓▓██░░░░░___▓▓█▄▄▓░░░░░░░░___░░░░█▓▓▓▓▓█▓▌
 _________█▓▓▓▓▓▓▓▓▓▓▓▓▓█░░█░░░___▓▓██░░░░░░░░▓▓▓▓__░░░░█▓▓▓▓██
 _________█▓▓▓▓▓▓▓▓▓▓▓▓▓█░███░░____▓░░░░░░░░░░░█▄█▓__░░░░█▓▓█▓█
 _________▐▓▓▓▓▓▓▓▓▓▓▓▓▓█░█████░░░░░░░░░░░░░░░░░█▓__░░░░███▓█
 __________█▓▓▓▓▓▓▓▓▓▓▓▓█░░███████░░░░░░░░░░░░░░░▓_░░░░░██▓█
 __________█▓▓▓▓▓▓▓▓▓▓▓▓█░░░███████░░░░░░░░░░░░░░_░░░░░██▓█
 __________█▓▓▓▓▓▓▓▓▓▓▓▓█░░░███████░░░░░░░░░░░░░░░░░░░██▓█
 ___________█▓▓▓▓▓▓▓▓▓▓▓▓█░░░░███████░░░░░░░░░░░█████░██░░░
 ___________█▓▓▓▓▓▓▓▓▓▓▓▓█░░░░░░__███████░░░░░███████░░█░░░░
 ___________█▓▓▓▓▓▓▓▓▓▓▓▓▓█░░░░░░█▄▄▄▀▀▀▀████████████░░█░░░░
 ___________▐▓▓▓▓▓▓▓▓▓▓▓▓█░░░░░░██████▄__▀▀░░░███░░░░░█░░░
 ___________▐▓▓▓▓▓▓▓▓▓▓▓█▒█░░░░░░▓▓▓▓▓███▄░░░░░░░░░░░░░░░______▄▄▄
 ___________█▓▓▓▓▓▓▓▓▓█▒▒▒▒█░░░░░░▓▓▓▓▓█░░░░░░░░░░░░░░░▄▄▄_▄▀▀____▀▄
 __________█▓▓▓▓▓▓▓▓▓█▒▒▒▒█▓▓░░░░░░░░░░░░░░░░░░░░░____▄▀____▀▄_________▀▄
 _________█▓▓▓▓▓▓▓▓▓█▒▒▒▒█▓▓▓▓░░░░░░░░░░░░░░░░░______▐▄________█▄▄▀▀▀▄__█
 ________█▓▓▓▓▓▓▓▓█▒▒▒▒▒▒█▓▓▓▓▓▓▓░░░░░░░░░____________█_█______▐_________▀▄▌
 _______█▓▓▓▓▓▓▓▓█▒▒▒▒▒▒███▓▓▓▓▓▓▓▓▓▓▓█▒▒▄___________█__▀▄____█____▄▄▄____▐
 ______█▓▓▓▓▓▓▓█_______▒▒█▒▒██▓▓▓▓▓▓▓▓▓▓█▒▒▒▄_________█____▀▀█▀▄▀▀▀___▀▀▄▄▐
 _____█▓▓▓▓▓██▒_________▒█▒▒▒▒▒███▓▓▓▓▓▓█▒▒▒██________▐_______▀█_____________█
 ____█▓▓████▒█▒_________▒█▒▒▒▒▒▒▒▒███████▒▒▒▒██_______█_______▐______▄▄▄_____█
 __█▒██▒▒▒▒▒▒█▒▒____▒▒▒█▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒____▒█▓█__▄█__█______▀▄▄▀▀____▀▀▄▄█
 __█▒▒▒▒▒▒▒▒▒▒█▒▒▒████▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█_______█▓▓█▓▓▌_▐________▐____________▐
 __█▒▒▒▒▒▒▒▒▒▒▒███▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒_______█▓▓▓█▓▌__▌_______▐_____▄▄____▐
 _█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒_____█▓▓▓█▓▓▌__▌_______▀▄▄▀______▐
 _█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒███████▓▓█▓▓▓▌__▀▄_______________▄▀
 _█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒███▒▒▒▒▒▒▒██▓▓▓▓▓▌___▀▄_________▄▀▀
 █▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒██▒▒▒▒▒▒▒▒▒▒▒▒▒█▓▓▓▓▓▀▄__▀▄▄█▀▀▀
 █▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒██▓▓▓▓██▄▄▄▀
 █▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒████
 █▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 _█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒▒█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▄▄▄▄▄
 _█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒▒▒█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒███▒▒▒▒▒▒██▄▄
 __█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒▒▒█▒▒▒▒▒▒▒▒▒▒▒▒███▒▒▒▒▒▒▒▒▒▒▒▒▒█▄
 __█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒▒▒█▒▒▒▒▒▒▒▒▒▒▒█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 __█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒▒▒█▒▒▒▒▒▒▒▒▒██▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 ___█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▒▒▒▒█▒▒▒▒▒▒▒▒█▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░▒▒▒▒▒▒▌
 ____█▒▒▒▒▒▒▒▒▒▒▒▒▒██▒▒▒▒▒▒▒█▒▒▒▒█▒▒▒▒▒▒█▒▒▒▒▒▒▒▒▒░░░░░░░░░░░░░▒▒▌
 ____█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█████████████▒▒▒▒▒█▒▒▒▒▒▒▒▒░░░░▒▒▒▒▒▒▒▒▒▒▒░▒▌
 _____█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█_______▐▒▒▒▒█▒▒▒▒▒▒▒░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░▌
 ______█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█________█▒▒█▒▒▒▒▒▒░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▌
 _______█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█________█▒█▒▒▒▒▒▒░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▌
 ________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█________█▒▒▒▒▒▒░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 _________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█________█▒▒▒▒░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 _________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█________█▒▒▒░░░░░░░▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▀
 __________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█_______█▒░░░▒▒▒▒▒░░░░░░░░▒▒▒█▀▀▀
 ___________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█_______█░▒▒▒▒▒▒▒▒▒▒▒▒▒░░░░█▀
 ____________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█_______█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█▀
 _____________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█_______█▒▒▒▒▒▒▒▒▒▒▒▒█▀
 _____________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█_______▀▀▀███████▀▀
 ______________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 _______________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 ________________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 _________________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 __________________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒██▒█
 ___________________█▒▒▒▒▒▒▒▒▒▒▒▒▒██▒▒▒▒█
 ___________________█▒▒▒▒▒▒▒▒████▒▒▒▒▒▒▒█
 ___________________█████████▒▒▒▒▒▒▒▒▒▒▒█
 ____________________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 ____________________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█
 _____________________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░▌
 _____________________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░▌
 ______________________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░▌
 _______________________█▒▒▒▒▒▒▒▒▒▒▒▒▒▒░░█
 ________________________█▒▒▒▒▒▒▒▒▒▒▒░░░█
 __________________________██▒▒▒▒▒▒░░░█▀
 _____________________________█░░░░░█▀
 _______________________________▀▀▀▀




 */
