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

static int RegisterFile[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char RegisterFileNames[32][6] = {"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};
static char RegisterNumberNames[32][4] = {"$0", "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15", "$16", "$17", "$18", "$19", "$20", "$21", "$22", "$23", "$24", "$25", "$26", "$27", "$28","$29","$30","$31"};
static char RegistersInUse[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char RegAccess[2] = {0,0};
static int LOCK = 0;


static int data_memory[1024]; // int values
static char* instructions[1024]; // instructions
static int controlRegisterZ[1024]; // Control RegisterZ

#define CACHE_LENGTH 16
#define CACHE_LINE_LENGTH 16
struct cache_line { // cache for the program
	int is_valid;
	int address_tag;
	int cacheLine[CACHE_LINE_LENGTH];
};
static struct cache_line cache[CACHE_LENGTH];

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
    int s_register1;
    int s_register2;
    char shamt;
};

struct i_type { // holds all i type values
    int *pc_ptr;
    instruction instr;
    char dest_register;
    int s_register;
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

/*void wrapper for getinstruction to use in pthreading*/
void* fetchvoid(void*);

/*getinstruction(int pc) is an interface for the control unit to access the instructions stored
in memory, by index specified by  the value stored in pc_ptr*/
char* getinstruction(int *pc_ptr);

/*void wrapper for decodeinstruction for use in pthreading*/
void* decodevoid(void*);

/*decodeinstruction(char*) takes an assembly instruction specified by instr and converts to an
Instruction_Type, converting the function, any registers, and any immediate values to integers
that are easier to operate on*/
struct Instruction_Type* decodeinstruction(char* instr);

/*void wrapper for execute for use in pthreading*/
void* executevoid(void*);

/*calls the appropriate execute_r, execute_i, execute_j*/
struct exmem* execute(struct Instruction_Type*);

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

/*void wrapper for memory_wb for use in pthreading*/
void* memory_rw_void(void*);

/*memory_rw(struct exmem*) takes a pointer to an exmem register struct and uses that to access the memory if needed
and create a memwb struct to pass data on to the register writeback pipeline phase*/
struct memwb* memory_rw(struct exmem*);

/*void wrapper for writeback for use in pthreading*/
void* writebackvoid(void*);

/*writeback(struct memwb*) takes a pointer to a memwb register struct and uses its data to write to
the appropriate registers, completing the execution of the instruction. Whew!*/
void writeback(struct memwb *memwb_ptr);

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

void check_cache(int);

void write_cache_line(int);

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
    
    readmips(argv[1]);     
    
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
    }
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
    int i; int j;   
    for(i=0;i<32;i++){
        printf("%s:%d,",RegisterFileNames[i],RegisterFile[i]);
    } printf("\n");
    for(i=0;i<32;i++){
        printf("%s:%d,",RegisterFileNames[i],RegistersInUse[i]);
    } printf("\n");
    for(i=0;i<70;i++){
        printf("%d,",data_memory[i]);
    } printf("\n");
    for(i=0;i<CACHE_LENGTH;i++){
        if(cache[i].is_valid==1){
            printf("cache %d of 16: ", i);
            for(j=0;j<CACHE_LINE_LENGTH;j++){
                printf("%d,",cache[i].cacheLine[j]);
            }
            printf("\n");
        }
    }

}

int controllogic(){
    int pc=0;
    void* cur_instruction[2]={NULL,NULL}; //analogous to if/id pipeline register
    void* instr[2]={NULL,NULL};
    void* exmem_ptr[2]={NULL,NULL};
    void* memwb_ptr[2]={NULL,NULL};
    struct Instruction_Type* instr_temp;
    struct exmem* exmem_temp;
    int lastpc=0;
	pthread_t threads[5];
    int willbreak=0; int reachedend;
    int numloops=0;
    int i;
    int lockedreg=0;
    memset(cache,0,sizeof(cache));
    printf("Control initiated:\n");

    while(1){
        if((instr[0]!=NULL)&&((*(struct Instruction_Type*)instr[0])).instr==Break){
            printf("bmbumbbumbumb\n");
            willbreak=2; reachedend=1;
            for(i=0;i<2;i++){
                cur_instruction[i]=NULL; instr[i]=NULL;
            }
        }
        printf("pc:%d\n",pc);
        printstate();

                
		pthread_create(&threads[4], NULL, writebackvoid, memwb_ptr[0]);
        pthread_create(&threads[0], NULL, fetchvoid, (void*) &pc);
		pthread_create(&threads[2], NULL, executevoid, instr[0]);
		pthread_create(&threads[3], NULL, memory_rw_void, exmem_ptr[0]);

		pthread_join(threads[4], NULL);

		pthread_create(&threads[1], NULL, decodevoid, cur_instruction[0]);

		// Wait for each thread to finish
		pthread_join(threads[0], &cur_instruction[1]);
		pthread_join(threads[1], &instr[1]);
		pthread_join(threads[2], &exmem_ptr[1]);
		pthread_join(threads[3], &memwb_ptr[1]);
        

        if(instr[1]!=NULL){
            instr_temp=instr[1];
            if((*instr_temp).instr<Addi){
                if(RegistersInUse[RegAccess[1]]!=0 || RegistersInUse[RegAccess[0]]!=0){
                    instr[0]=NULL; instr[1]=NULL; cur_instruction[1]=cur_instruction[0]; pc = lastpc;
                }
                if((*(struct r_type*)instr_temp).dest_register!=0){
                    lockedreg=(*(struct r_type*)instr_temp).dest_register;
                    RegistersInUse[lockedreg]=1;
                } 
            } else if((*instr_temp).instr<J){ 
                if(RegistersInUse[RegAccess[1]]!=0 || RegistersInUse[RegAccess[0]]!=0){
                    instr[0]=NULL; instr[1]=NULL; cur_instruction[1]=cur_instruction[0]; pc = lastpc;
                }
                if(LOCK!=0 && (*(struct i_type*)instr_temp).dest_register!=0){
                    lockedreg=(*(struct i_type*)instr_temp).dest_register;
                    RegistersInUse[lockedreg]=1;
                }
            } else if((*instr_temp).instr==Jal){
                RegistersInUse[31]=1;
                lockedreg=31;
            }
        }   
        cur_instruction[0]=cur_instruction[1];
        instr[0]=instr[1];
        exmem_ptr[0]=exmem_ptr[1];
        memwb_ptr[0]=memwb_ptr[1];
        if(instr[0]!=NULL){
            instr_temp=instr[0];
            (*instr_temp).pc_ptr = &pc;
        }
        if(exmem_ptr[0]!=NULL){
            exmem_temp = exmem_ptr[0];
            if((*exmem_temp).will_branch!=0){
                if(lockedreg!=0){
                    RegistersInUse[lockedreg]=0;
                }   
                cur_instruction[0]=NULL; instr[0]=NULL;
                pc=(*exmem_temp).will_branch;
            }
        }
        lastpc = pc;
        LOCK=0; lockedreg=0;
        numloops++;
        if(numloops>1300){
            //break;
        }
        if(willbreak>0){
            willbreak--;
            for(i=0;i<2;i++){
                cur_instruction[i]=NULL; instr[i]=NULL;
            }
        } else if((willbreak==0)&&(reachedend==1)){
            break;
        }
    }
    for(numloops=0;numloops<CACHE_LENGTH;numloops++){
        if(cache[numloops].is_valid==1){
            write_cache_line(numloops);
        }
    }
    printstate();
    return 0;
}

void* fetchvoid(void* void_pc){
    return (void*)getinstruction((int*)void_pc);
}

char* getinstruction(int *pc_ptr){
    int pc = *pc_ptr;
    char* instr;
    printf("fpc:%d\n",pc);
    if(instructions[pc]!=NULL){
        instr = instructions[pc];
    } else{
        instr = "";
    }
    printf("f:%s\n",instr);
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

    printf("dec:\n");
    if(instr==NULL){
        printf("nulldec\n");
        return NULL;
    }
    printf("dec:%s\n",instr);
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
        printf("decr:inum:%d,dest:%d,s1:%d,s2:%d,shamt:%d\n",(*r_instr_ptr).instr,(*r_instr_ptr).dest_register,(*r_instr_ptr).s_register1,(*r_instr_ptr).s_register2,(*r_instr_ptr).shamt);
        instr_ptr=(struct Instruction_Type*)r_instr_ptr;
    } else if(instrnum<J){
        i_instr_ptr=make_i_type(instrnum, instr, ptr);
        printf("deci:inum:%d,dest:%d,s:%d,imm:%d\n",(*i_instr_ptr).instr,(*i_instr_ptr).dest_register,(*i_instr_ptr).s_register,(*i_instr_ptr).immediate);
        instr_ptr=(struct Instruction_Type*)i_instr_ptr;
    } else if (instrnum<Break){
        j_instr_ptr=make_j_type(instrnum, instr, ptr);
        printf("decj:inum:%d,jump:%d\n",(*j_instr_ptr).instr,(*j_instr_ptr).jump_to);
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
    int *pc_ptr;
    int new_pc;

    printf("exe\n");
    if(instr==NULL){
        printf("exenull\n");
        return NULL;
    }
    pc_ptr=(*instr).pc_ptr;
    new_pc = *pc_ptr;
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
        printf("asfwefuahwefdad\n");
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
    
    printf("exr:instr:%d,alu:%d,dest:%d,branch:%d\n",(*exmem_ptr).instrnum,(*exmem_ptr).alu_result,(*exmem_ptr).dest_register,(*exmem_ptr).will_branch);
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
    printf("exi:instr:%d,alu:%d,dest:%d,branch:%d\n",(*exmem_ptr).instrnum,(*exmem_ptr).alu_result,(*exmem_ptr).dest_register,(*exmem_ptr).will_branch);
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
    
    printf("exj:instr:%d,alu:%d,dest:%d,branch:%d\n",(*exmem_ptr).instrnum,(*exmem_ptr).alu_result,(*exmem_ptr).dest_register,(*exmem_ptr).will_branch);
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
    struct exmem exmem_reg;
    struct memwb *memwb_ptr = (struct memwb*)calloc(1, sizeof(struct memwb));
    struct memwb memwb_reg = *memwb_ptr;

    printf("mem\n");
    if(exmem_ptr==NULL){
        printf("mmenyll\n");
        return NULL;
    }
    exmem_reg=*exmem_ptr;
    printf("exmem:instr:%d,dest:%d,alu:%d\n",exmem_reg.instrnum,exmem_reg.dest_register,exmem_reg.alu_result);
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
    printf("mem:instr:%d,dest:%d,val:%d\n",(*memwb_ptr).instrnum,(*memwb_ptr).dest_register,(*memwb_ptr).value);
    return memwb_ptr;
}

void* writebackvoid(void* memwb_void){
    writeback((struct memwb*) memwb_void);
    return NULL;
}

void writeback(struct memwb* memwb_ptr){
    struct memwb memwb_reg;
    printf("wb\n");
    if(memwb_ptr!=NULL){
        memwb_reg = *memwb_ptr;
        printf("memwb:instr:%d,dest:%d,val:%d\n",memwb_reg.instrnum,memwb_reg.dest_register,memwb_reg.value);
        if(memwb_reg.instrnum==Mfhi){
            mfhi(memwb_reg.dest_register);
        } else if(memwb_reg.instrnum==Mflo){
            mflo(memwb_reg.dest_register);
        } else if(memwb_reg.instrnum==MtcZ){
            mtcZ(memwb_reg.dest_register,memwb_reg.value);
        } else{
            register_write(memwb_reg.dest_register,memwb_reg.value);
        }
        if(memwb_reg.dest_register!=0){
            RegistersInUse[memwb_reg.dest_register]=0;
        }
    }
    
    free(memwb_ptr);
}



// decode helper functions
struct r_type* make_r_type(int instrnum, char* instr, int* ptr){
    struct r_type *r_type_ptr=(struct r_type*)calloc(1, sizeof(struct r_type));
    int reg1=0; int reg2=0;
    
    (*r_type_ptr).instr=instrnum; (*r_type_ptr).pc_ptr = NULL;
    if(instrnum==Jr) { // get address from RF
        reg1=nextregister(instr, ptr);
        (*r_type_ptr).s_register1=RegisterFile[reg1];
    } else if(instrnum>=Mfhi) { //mfhi, mflo save to register
        (*r_type_ptr).dest_register=nextregister(instr, ptr);
    } else if(instrnum==MfcZ){ // save to in dest, save from location in s_r1
        (*r_type_ptr).dest_register=nextregister(instr, ptr);
        reg1 = nextregister(instr, ptr);
        (*r_type_ptr).s_register1=controlRegisterZ[RegisterFile[reg1]];
    } else if(instrnum==MtcZ){ // save value in dest, save to location in sr1
        reg1=nextregister(instr, ptr);
        reg2=nextregister(instr, ptr);
        (*r_type_ptr).dest_register=RegisterFile[reg1]; //value
        (*r_type_ptr).s_register1=RegisterFile[reg2]; // location
    } else if(instrnum>=Mult){ // all mult, div operations, save values to 2 source registers
        reg1 = nextregister(instr, ptr);
        reg2 = nextregister(instr, ptr);
        (*r_type_ptr).s_register1=RegisterFile[reg1];
        (*r_type_ptr).s_register2=RegisterFile[reg2];
    } else if(instrnum>=Sll){ // shift operations - dest, value of s_r, shamt
        (*r_type_ptr).dest_register=nextregister(instr, ptr);
        reg1 = nextregister(instr, ptr);
        (*r_type_ptr).s_register1=RegisterFile[reg1];
        (*r_type_ptr).shamt=nextint(instr, ptr);
    } else{ //all normal 3 register operations - values of 2 source regs
        (*r_type_ptr).dest_register=nextregister(instr, ptr);
        reg1 = nextregister(instr, ptr); reg2 = nextregister(instr, ptr);
        (*r_type_ptr).s_register1=RegisterFile[reg1];
        (*r_type_ptr).s_register2=RegisterFile[reg2];
    }
    LOCK=1;
    RegAccess[0]=reg1;
    RegAccess[1]=reg2;
    printf("instr:%d,dest:%d,s1:%d,s2:%d,shamt:%d\n",(*r_type_ptr).instr,(*r_type_ptr).dest_register,(*r_type_ptr).s_register1,(*r_type_ptr).s_register2,(*r_type_ptr).shamt);
    return r_type_ptr;
}

struct i_type* make_i_type(int instrnum, char* instr, int* ptr){
    struct i_type *i_type_ptr=(struct i_type*)calloc(1, sizeof(struct i_type));
    struct indexed_register *indreg_ptr;
    int reg1 = 0; int reg2 = 0;
    (*i_type_ptr).instr=instrnum; (*i_type_ptr).pc_ptr = NULL;
    
    if(instrnum==Lui){ // lui needs dest and imm
        (*i_type_ptr).dest_register=nextregister(instr, ptr);
        (*i_type_ptr).immediate=nextint(instr, ptr);
        LOCK=1;
    } else if(instrnum>=Sw){ // stores need value of dest, s, imm
        reg1 = nextregister(instr, ptr);
        (*i_type_ptr).dest_register=RegisterFile[reg1];
        indreg_ptr = nextindexedregister(instr, ptr);
        reg2 = (*indreg_ptr).mem_register;
        (*i_type_ptr).s_register=RegisterFile[reg2]; (*i_type_ptr).immediate=(*indreg_ptr).index;
        free(indreg_ptr);
    } else if(instrnum>=Lw){ //writes to dest_reg, needs value of s, imm
        (*i_type_ptr).dest_register=nextregister(instr, ptr);
        indreg_ptr = nextindexedregister(instr, ptr);
        reg1 = (*indreg_ptr).mem_register;
        (*i_type_ptr).s_register=RegisterFile[reg1]; (*i_type_ptr).immediate=(*indreg_ptr).index;
        free(indreg_ptr);
        LOCK=1;
    } else if(instrnum>=Beq){
        reg1 = nextregister(instr, ptr); reg2 = nextregister(instr, ptr);
        (*i_type_ptr).dest_register=RegisterFile[reg1];
        (*i_type_ptr).s_register=RegisterFile[reg2]; (*i_type_ptr).immediate=nextjumploc(instr,ptr);
    } else{
        (*i_type_ptr).dest_register=nextregister(instr, ptr);
        reg1 = nextregister(instr, ptr);
        (*i_type_ptr).s_register=RegisterFile[reg1]; (*i_type_ptr).immediate=nextint(instr, ptr);
        LOCK=1;
    }
    RegAccess[0]=reg1;
    RegAccess[1]=reg2;
    printf("instr:%d,dest:%d,s:%d,imm:%d\n",(*i_type_ptr).instr,(*i_type_ptr).dest_register,(*i_type_ptr).s_register,(*i_type_ptr).immediate);
    return i_type_ptr;
}

struct j_type* make_j_type(int instrnum, char* instr, int* ptr){
    struct j_type *j_type_ptr=(struct j_type*)calloc(1, sizeof(struct j_type));

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

void check_cache(int location){
    int slot = (location/(4*CACHE_LINE_LENGTH))%CACHE_LENGTH;
    int tag = location/(4*CACHE_LINE_LENGTH*CACHE_LENGTH);
    struct cache_line curcache = cache[slot];
    int is_hit = 0;
    int i;
    if(curcache.is_valid==1 && curcache.address_tag==tag){
        is_hit = 1;
    }
    if(is_hit==0){
        if(curcache.is_valid==1){
            for(i=0;i<CACHE_LINE_LENGTH;i++){
                data_memory[(location/(4*CACHE_LINE_LENGTH))*CACHE_LINE_LENGTH+i]=curcache.cacheLine[i]; // rewrite memory
            }
        }
        cache[slot].is_valid = 1;
        cache[slot].address_tag = tag;
        for(i=0;i<CACHE_LINE_LENGTH;i++){
            curcache.cacheLine[i]=data_memory[(location/(4*CACHE_LINE_LENGTH))*CACHE_LINE_LENGTH+i]; // rewrite memory
        }
    }
}

void write_cache_line(int slot){
    struct cache_line curcache = cache[slot];
    int mostsig=((curcache.address_tag*CACHE_LENGTH)+slot)*CACHE_LINE_LENGTH;
    int i;
    for(i=0;i<CACHE_LINE_LENGTH;i++){
        data_memory[mostsig+i]=curcache.cacheLine[i];
    }
}

// Read in Load Word Memory (Memory Stage)
int lw_read(int registerAndIndex) {
    
	int slot = (registerAndIndex/(4*CACHE_LINE_LENGTH))%CACHE_LENGTH;
    int offset = (registerAndIndex/4)%CACHE_LINE_LENGTH;

    check_cache(registerAndIndex);
    return cache[slot].cacheLine[offset];
}

// Read in Load Halfword Memory (Memory Stage)
short lh_read(int registerAndIndex) {
	// Need to figure out which half to get (distinguish from each other)
	int slot = (registerAndIndex/(4*CACHE_LINE_LENGTH))%CACHE_LENGTH;
    int offset = (registerAndIndex/4)%CACHE_LINE_LENGTH;
    int a; short b;

    check_cache(registerAndIndex);
	
	a = cache[slot].cacheLine[offset];
	b = (short)a; //TODO: make this get the right half
    return b;
}

unsigned short lhu_read(int registerAndIndex){
	int slot = (registerAndIndex/(4*CACHE_LINE_LENGTH))%CACHE_LENGTH;
    int offset = (registerAndIndex/4)%CACHE_LINE_LENGTH;
    int a; unsigned short b;

    check_cache(registerAndIndex);

    a = cache[slot].cacheLine[offset];
    b = (unsigned short)b;

    return b;
}


// Read in Byte Memory (Memory Stage)
char lb_read(int registerAndIndex) {
	int slot = (registerAndIndex/(4*CACHE_LINE_LENGTH))%CACHE_LENGTH;
    int offset = (registerAndIndex/4)%CACHE_LINE_LENGTH;
    int a; char b;

    check_cache(registerAndIndex);

    a = cache[slot].cacheLine[offset];
    b = (char)b;

    return b;
}

unsigned char lbu_read(int registerAndIndex){
	int slot = (registerAndIndex/(4*CACHE_LINE_LENGTH))%CACHE_LENGTH;
    int offset = (registerAndIndex/4)%CACHE_LINE_LENGTH;
    int a; unsigned char b;

    check_cache(registerAndIndex);

    a = cache[slot].cacheLine[offset];
    b = (unsigned char)b;

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
	int slot = (registerAndIndex/(4*CACHE_LINE_LENGTH))%CACHE_LENGTH;
    int offset = (registerAndIndex/4)%CACHE_LINE_LENGTH;
	check_cache(registerAndIndex);
    
    cache[slot].cacheLine[offset]=register1;
}


// Store Halfword (WriteBack Stage)
void sh(int register1, int registerAndIndex) {
	int slot = (registerAndIndex/(4*CACHE_LINE_LENGTH))%CACHE_LENGTH;
    int offset = (registerAndIndex/4)%CACHE_LINE_LENGTH;
	if (registerAndIndex % 4 == 2 || registerAndIndex % 4 == 3) {
		cache[slot].cacheLine[offset] = ((cache[slot].cacheLine[offset] >> 16) << 16) + register1;
	} else if (registerAndIndex % 4 == 0 || registerAndIndex % 4 == 1) {
		cache[slot].cacheLine[offset] = ((cache[slot].cacheLine[offset] << 16) >> 16) + (register1 << 16);
	}
}

// Store Byte (WriteBack Stage)
void sb(int register1, int registerAndIndex) {
	int slot = (registerAndIndex/(4*CACHE_LINE_LENGTH))%CACHE_LENGTH;
    int offset = (registerAndIndex/4)%CACHE_LINE_LENGTH;
	
    if (registerAndIndex % 4 == 0) {
		cache[slot].cacheLine[offset] = ((cache[slot].cacheLine[offset] << 8) >> 16) + (register1 << 24);
	} else if (registerAndIndex % 4 == 1) {
		cache[slot].cacheLine[offset] = ((cache[slot].cacheLine[offset] << 16) >> 16) + ((cache[slot].cacheLine[offset] >> 24) << 24) + (register1 << 16);
	} else if (registerAndIndex % 4 == 2) {
		cache[slot].cacheLine[offset] = ((cache[slot].cacheLine[offset] << 24) >> 24) + ((cache[slot].cacheLine[offset] >> 16) << 16) + (register1 << 8);
	} else if (registerAndIndex % 4 == 3) {
		cache[slot].cacheLine[offset] = ((cache[slot].cacheLine[offset] >> 8) << 8) + register1;
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



 /*

 _____________________________________________________________________
|                                                                      |
|  =================================================================== |
| |%/^\\%&%&%&%&%&%&%&%&{ Federal Reserve Note }%&%&%&%&%&%&%&%&//^\%| |
| |/inn\)===============------------------------===============(/inn\| |
| |\|UU/              { UNITED STATES OF AMERICA }              \|UU/| |
| |&\-/     ~~~~~~~~   ~~~~~~~~~~=====~~~~~~~~~~~  P8188928246   \-/&| |
| |%//)     ~~~_~~~~~          // ___ \\                         (\\%| |
| |&(/  13    /_\             // /_ _\ \\           ~~~~~~~~  13  \)&| |
| |%\\       // \\           :| |/ ~ \| |:  3.21  /|  /\   /\     //%| |
| |&\\\     ((iR$)> }:P ebp  || |"- -"| ||        || |||| ||||   ///&| |
| |%\\))     \\_//      sge  || (|e,e|? ||        || |||| ||||  ((//%| |
| |&))/       \_/            :| `._^_,' |:        || |||| ||||   \((&| |
| |%//)                       \\ \\=// //         || |||| ||||   (\\%| |
| |&//      R265402524K        \\U/_/ //   series ||  \/   \/     \\&| |
| |%/>  13                     _\\___//_    1932              13  <\%| |
| |&/^\      Treasurer  ______{Franklin}________   Secretary     /^\&| |
| |/inn\                ))--------------------((                /inn\| |
| |)|UU(================/ ONE HUNDERED DOLLARS \================)|UU(| |
| |{===}%&%&%&%&%&%&%&%&%a%a%a%a%a%a%a%a%a%a%a%a%&%&%&%&%&%&%&%&{===}| |
| ==================================================================== |
|______________________________________________________________________|


                          ~   CACHE MONEY!   ~


 */


/*

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++;"^~~555^"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++?~||||||||hh|^+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++"5|||||||||hhhh]^?^^~|||5~"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++;5h|||||||||hhhhhh|||||||||hh|^++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++??"^~~|h|||hhhhh||||||||||hhhhqh";+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++;;;++++;^5hhhqhh|||||||||hhhhq]pph5"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++++++;?;;;;;;;??"~|5""^~5|||||hhhhqq]p0phh|^^?;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++;"]0^?????""^^"?;;+++;?^5|hhhhq]p000phhhhq]q^++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++?qO]h??"""^^~^??;;;;;;;;??~|q]pp00000hhhhhqpO05++++++++++++++++++++++++++++++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++++??00p|"""^^~~5""p0^???????""^5q0000000hhhhhhq]0QO|?+++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++?"^~55|||||55|h|5qO0^^^^~~5|5"]0]|??"""""^^~~5q0000phhhhhhhhh]pOQQq;+++++++++++++++++++++++++++++++++++++++++++++++++
++++++++++++++;"~5||||||||||||||||||hhh|||55|||5^0pO5"""^^^^^~~5|h]00q||||hhhhhhh]pOQQ++++++++++++++++++++++++++++++++++++++++++++++++++
++++++++++++?|0]||||||||||||||||||||||||hhhhhhh|~]Q0^^^^~~~555|hqqp]|||||||hhhhhhq]p0O++++++++++++++++++++++++++++++++++++++++++++++++++
++++++++++^||qh||||||||||||hh|||||||||||hhhhhhhhhhq5~~~~55||hhhqq]h|||||||hhhhhhhh]pp"++++++++++++++++++++++++++++++++++++++++++++++++++
++++++++?||||||||||||||]00O0]||||||||hhhhhhhhhhhhhhqh||||||hhhhhhhhhhhh||hhhhhhhhhq]p]?+++++++++++++++++++++++++++++++++++++++++++++++++
+++++++~||||||||||||||||||||||||||hhhhhhhhhhhhhhhhhqh||||||hhhhhhhhhhhqqhhhhhhhhhhq]pp];++++++++++++++++++++++++++++++++++++++++++++++++
+++++;5|||||||||||||||||||||||||hhhhhhhhhhhhhhhhhhhh||||||hhhhhhhhhhhqqq]]qhhhhhhqq]ppph++++++++++++++++++++++++++++++++++++++++++++++++
++++;5h||||||||||||||||||h|hhhhhhhhhhhhhhhhhhhhqhhhhh||||||hhhhhhhhhqqq]]]p]qhhqqq]]ppp0"+++++++++++++++++++++++++++++++++++++++++++++++
++++5hh||||||||||||||||hhhhhhhhhhhhhhhhhhhhhqqqhhhhhhhhhhhhhhhhhhhhqqq]]]ppppqqqq]]pppp05+++++++++++++++++++++++++++++++++++++++++++++++
+++~hhhh|||||||||||||hhhhhhhhhhhhhhhhhhhhhqq]qhhhhhhhhhhhhhhhhhhhqqq]]]]pppp00]]]]pppp005+++++++++++++++++++++++++++++++++++++++++++++++
+++hhhhhhhh||h|hhhhhhhhhhhhhhhhhhhhhhhhhqq]]qhhhhhhhhhhhhhh|~^"?????"^5qpp000Op]ppppp0005+++++++++++++++++++++++++++++++++++++++++++++++
++^hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhqq]]p]hhhhhhhhhhhh5";;;;;;????"""^~q00OQOppppp0000"+++++++++++++++++++++++++++++++++++++++++++++++
++?hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhqqq]]ppqhhhhhhhhh|^;;;;;;?????"""^^~~5qQQQOpppp0000~++++++++++++++++++++++++++++++++++++++++++++++++
+++5hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhqqqq]]pppqhhhhhhhh~;;;;;?????""""^^~~55||OQQ0000000p"+++++++++++++++++++++++++++++++++++++++++++;?;+++
+++?hhhhhhhhhhhhhhhhhhhhhhhhhhqqqqqq]]]ppppphqqqqq|?????????""""^^~~~55||||OQ0000000q?++++++++++++++++++++++++++++++++++++++++++"~|||"++
++++?hqqhhhhhhhhhhhhhhhhhhqqqqqqqq]]]]ppppppqqqqq~???"""""""^^^~~~55||||||]Q000000]^+++++++++++++++++++++++++++++++++++++++++;^5|||||^++
++++++~]]]qqqqqqqqqqqqqqqq]]]]]]]]]]pppppppppqqh""""^^^^^^^~~~555||||||||]O0000p5?+++++++++++++++++++++++++++++++++++++++++?~||||||||^++
+++++++?|ppp]]]]]]]]]]]]pppppppppppppppppppppp]~~~~~~~~55555|||||||||||qp00000~;++++++++++++++++++++++++++++++++++++++++;^5|||||||||h+++
+++++++++?|pppppppppppppppppppppppppppppppppppp]|5555|555|||||||||||hqp0000OQQQQQpq~+++++++;;;++;;+;+++++++++++++++++;?~||||||||||hq~+++
+++++++++++;^hppppppppppppppppppppppppppppppppq5^~~555|5||||||||||5|q0000000QQQQQQQQ++++++;;;?"""??;;+++++;+;++++;?^~||||||||||||hh]++++
++++++++++++++;^|qpppppppppppppppppppppppp]h5^^^^^^^^^~~~~~~~~~~~|]0000000000OQQQQQQ5^~h]]]00000ppq|5~^^?;++++;;"5||||||||||||||hhp"++++
+++++++++++++++++++?"~5|qq]]ppp]]q]]]qh|5^^^^^^^^^^^^^^^^^^^~5hqpp0000000000000QQQQQh~~~~|qpOOOOOOO0p]qh|55~^?;+++;"5||||||||||hhp|+++++
+++++++++++++++++++++++++++++++++++;??"""^^^^^^^^^^^^^^^^~~~qppppp000000000000000OOQ]h|~~~~~5q0OOOOOOO0p]]]qh55~";++;"||hhhhhhhhpq++++++
+++++++++++++++++++++++++++++++++++++++++++++^^^^^^^^^^^^~~~~qppppp000000000000000000000ph5~~~~]OOOOOOOO0]]]]]h5|h^;++;~hhhhhhhp];++++++
+++++++++++++++++++++++++++++++++++++++++++++~^^^^^^^^^^^~~~~~qppppp00000000000000000000000]~~~~]OOOOOOOO0]]]]]]h5|5;+++^hhhhqp0"+++++++
+++++++++++++++++++++++++++++++++++++++++++++"^^^^^^^^^^^^~~~~~qpppp000000000000000000000000]~~~~0OOOOOOOOp]]]]]]]|55;+++^hhqp0^++++++++
+++++++++++++++++++++++++++++++++++++++++++++?^^^^^^^^^^^^^~~~~~]pppp000000000000000000000000h~~~|OOOOOOOO0]]]]]]]]|5~;++;|qp0|+++++++++
+++++++++++++++++++++++++++++++++++++++++++++?^^^^^^^^^^^^^^~~~~5ppppp00000000000000000000000p~~~^]OOOOOOO0]]]]]]]]pq5^+++^p0q++++++++++
+++++++++++++++++++++++++++++++++++++++++++++?^^^^^^^^^^^^^^^~~~~|ppppp00000000000000000000000h~~~5OOOOOOO0ppppppppp]|"+++^00"++++++++++
+++++++++++++++++++++++++++++++++++++++++++++q^^^^^^^^^^^^^^^^~~~~qppppp0000000000000000000000p~~~~]OOOO0ppppppppppp|~;++;q0~+++++++++++
+++++++++++++++++++++++++++++++++++++++++++++05^^^^^^^^^^^^^^^^~~~~qpppppp000000000000000000000h~~^^000ppppppppppppq^;++?h05++++++++++++
+++++++++++++++++++++++++++++++++++++++++++;~0]^^^^^^^^^^^^^^^^^~~~~hppppppp0000000p000000000000~~;;"]ppppppppppppq"++;~]0~+++++++++++++
++++++++++++++++++++++++++++++++++++++++;~q0000~^^^^^^^^^^^^^^^^^^~~~5qppppppp00000000000000p000p~?+;?5qq]]]]qh|5";;?~]0h?++++++++++++++
++++++++++++++++++++++++++++++++++++++;50000000h^^^^^^^^^^^^^^^^^^^~~~~5hpppppp00ppp00000pqq]00000h^???;;;;;;;??""5qp0]^++++++++++++++++
+++++++++++++++++++++++++++++++++++++50000000000~^^^^^^^^^^^^^^^^^^^^~~~~~5h]p0ppppppp0]qhq]p0000000pqh||55|||hqp000]^++++++++++++++++++
+++++++++++++++++++++++++++++++++++?]0000000000p5"^^^^^^^^^^^^^^^^^^^^^~~~~~~50pppppp]hhhh]p0000000000000000000000q^++++++++++++++++++++
++++++++++++++++++++++++++++++++++?]00pp]]pp0h^+++?^^^^^^^^^^^^^^^^^^^^^^~~~~~00pp]qhhhhh]pp000000000000000000005?++++++++++++++++++++++
+++++++++++++++++++++++++++++++++^|h|||||hhhq++++++;"^^^^^^^^^^^^^^^^^^~~~~~5|qqh||hhhhh]p00000000000]hqp0000000++++++++++++++++++++++++
++++++++++++++++++++++++++++++++~|||||||hhhq]5+++++++?^^^^^^^^^^^^^^^~~~~5|||||||||hhhh]p0000000000phhhqqq]p000p++++++++++++++++++++++++
+++++++++++++++++++++++++++++++"hhhhhhhhhhhq]5+++++++++?^^^^^^^^^^^^~~5||||||||||hhhq]pp00000000000qhhhqq]]pp005++++++++++++++++++++++++
+++++++++++++++++++++++++++++++~hhhhhhhhhhhq]];+++++++++;"^^^^^^^^^~5|||||||||hhq]ppp0000000000000qhhhq]]ppp000;++++++++++++++++++++++++
+++++++++++++++++++++++++++++++^hhhhqhhhhhq]ppq++++++++++++?"^^^^~5||||||||hhq]p00q|p00000000000pqhhhqq]ppp000^+++++++++++++++++++++++++
+++++++++++++++++++++++++++++++?hhqq]0p]]ppp00q+++++++++++++h0q5|||||||||hh]pp0ph~5p0pp0p000000]hhhhqq]pp0000~++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++;5qppp0q|||||^+++++++?"^~|hh0q||||||||||hqp000qhhhq0pppppp0p]qhqqqqqq]pp0000~+++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++;"?++++++++++"~h]0OOOOOOO]h|||||||hhhh]pQQQQQQQOppppppqhhhqq]]]]]]pp0000^++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++++"|]]pOOOOOOOQQ0hhhhhhhhhhhhq]pOOOOO000pppppphqqq]]ppppppp000]"+++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++?hp00OOOOOQQQQQQ0hhhhhhhhhhhhh]pp000000O0ppppp]]]ppppppppp000h+++++++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++++++++++++?p0OOOOOOOQQQQ0]]]hhhhhqhhhhhq]]p000000OOQp]pppppppp0000000000Oq^+++++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++++++++++++50000OOOOQQQQO0p]]qhhqq]0p]]]pp00000p00QQp]]0O0ppp000000000OQQOOO]++++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++++++++++++^0000000OOOOOOOOOQO0ppppp00000p00000000OOp]]]pQQQOOOOOOOQQQOOO0000^+++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++"p000000000000000000pppp00000OO0pp]]q]0OQOppp0OQQ0000ppp]]]]]]]ph++++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++++++++++++++"q000000000000000000000OO00q~+++++?~q0O00p0OOO0]]]]]]]]]]]pp00Op"+++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++++++++++++++++;~|]p0000000000pp]]qh5~";++++?5pOQOp]]]p0p]]]]]]]]]]]pp0OOOOOOO+++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++++++++++??"^^^^"""""?;++++++++"h0OOO]]]]]]]]]]]]]]]]]]]]]]]pOOOOOO;++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|0OOOQQp]]]]]]]]]]]]]]]]]]]]]]pp0OO]?+++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++500OOQOOOQ0p]]]]]]]]]]]]]]]]]]pp0Oq5+++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++p0000pppp]]]|"~]]]]]]]]]]]]]]pp00;+++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|0p]]]]]]]]]]]]]]]]]]]]]]]]ppp00"++++++++++++++++++++++++++++++++
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|]]]]]]]]]]]]]]]]]]]]]]pppp00p?+++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"qppp]]]]]]]]]]]pppppppp00p|;++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"~|qq]ppppppppppp000pq5";++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++;?"^^~5hh|||5^"?+++++++++++++++++++++++++++++++++++++++++


*/
