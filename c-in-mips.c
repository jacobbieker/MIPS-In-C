#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

static int RegisterFile[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char RegisterFileNames[31][4] = {"$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};

static int data_memory[1024]; // int values
static char* instructions[1024]; // instructions

static char* jump_names[128]; // jump location names and addresses
static int jump_locations[128];

static int LO;
static int HI;

// instruction name enum and array
typedef enum {Add, Addu, Sub, Subu, And, Or, Nor, Xor, Slt, Sltu, Srlv, Sllv, Srav, Sll, Srl, Sra, Mult, Multu, MDiv, Divu, MfcZ, MtcZ, Mfhi, Mflo, Jr, Addi, Addiu, Andi, Ori, Slti, Beq, Bne, Lw, Lh, Lhu, Lb, Lbu, Sw, Sh, Sb, Lui, J, Jal} instruction;
static char* instr_name_list[44] = {"add", "addu", "sub", "subu", "and", "or", "nor", "xor", "slt", "sltu", "srlv", "sllv", "srav", "sll", "srl", "sra", "mult", "multu", "div", "divu", "mfcZ", "mtcZ", "mfhi", "mflo", "jr", "addi", "addiu", "andi", "ori", "slti", "beq", "bne", "lw", "lh", "lhu", "lb", "lbu", "sw", "sh", "sb", "lui", "j", "jal"};

struct Instruction_Type { // generic instruction type - holds instruction only
    instruction instr;
};

struct r_type { // holds all r type values
    instruction instr;
    char dest_register;
    char s_register1;
    char s_register2;
    char shamt;
};

struct i_type { // holds all i type values
    instruction instr;
    char dest_register;
    char s_register;
    int immediate;
};

struct j_type{ // holds all j type values
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
    int index;
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
int safe_add(int a, int b);

int safe_sub(int a, int b);

// Start of Arithmetic functions
int add(int register2, int register3);

unsigned int addu(int register2, int register3);

int sub(int register2, int register3);

unsigned int subu(int register2, int register3);

int addi(int register2, int number);

unsigned int addiu(int register2, int number);

void mult(int register2, int register3);

void multu(int register2, int register3);

void mdiv(int register2, int register3);

void divu(int register2, int register3);

// Start of Logical functions
int and(int register2, int register3);

int andi(int register2, int number);

int ori(int register2, int number);

int xor(int register2, int register3);

int nor(int register2, int register3);

int slt(int register2, int register3);

unsigned int sltu(int register2, int register3);

int slti(int register2, int number);


/*america() frees all memory that has been malloc'ed and not otherwise freed
IF YOU HAVE CALLED malloc() MAKE SURE TO FREE THAT MEMORY. DO IT HERE IF THERE
IS NO BETTER PLACE. Thanks.
Current memory that is deallocated here:
instructions: the array holding all instructions in char* form
jump_names: the array holding all jump names in char* form
*/
void america();

int main() {
int i=0; char *a;
    
    readmips("bubble.asm");
    while(instructions[i]!=NULL){
        printf("%s\n",instructions[i]);
        ++i;
    }
    i=0;
    while(jump_names[i]!=NULL){
        printf("%d, %s\n", jump_locations[i], jump_names[i]);
        i++;
    }

    RegisterFile[7]=2; RegisterFile[8]=4;
    execute_r((struct r_type*)decodeinstruction("add $t2, $t1, $t0"));
    execute_i((struct i_type*)decodeinstruction("addi $t2, $t2, 10"));
    execute_j((struct j_type*)decodeinstruction("jal    loop"), 20);
    execute_r((struct r_type*)decodeinstruction("jr $t1"));
    execute_i((struct i_type*)decodeinstruction("sw $t0 12($t1)"));
    execute_i((struct i_type*)decodeinstruction("lw $t0 12($t1)"));
    printf("%d %d\n", HI, LO);
    execute_r((struct r_type*)decodeinstruction("mult $t0 $t1"));
    printf("%d %d\n", HI, LO);
    /*a="lw $t5 -4($fp)";
    printf("%s\n", a); decodeinstruction(a);
    a="sw $t6 0($s0)";
    printf("%s\n", a); decodeinstruction(a);
    a="sub $gp $s7 $t8";
    printf("%s\n", a); decodeinstruction(a);
    a="bne $t0,    $t1, loop";
    printf("%s\n", a); decodeinstruction(a);
    */
    america();
    return 0;
}

void readmips(char* filename){
    FILE * assembly = fopen(filename, "r");
    int lineno=0; int charno=0; int i=0; int spacebreak=0; int jumpno=0;
    char buffer[128];
    char* temp; char* temploc;
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
            temp = (char*)malloc(charno+1);
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
                    spacebreak+=i;
                }
                *(temp+i-spacebreak)=buffer[i];
                i++;
            }
            if (charno!=0 && spacebreak!=charno){
                instructions[lineno]=temp;
                lineno++; //rewrite over memory if the line has no instruction
            }
        }
    }
}

int controllogic(){
    int pc=0;
    char *cur_instruction; //analogous to if/id pipeline register
    struct j_type *j_instr_ptr;
    struct i_type *i_instr_ptr;
    struct r_type *r_instr_ptr;
    struct Instruction_Type* instr;

    while(1){
        cur_instruction = getinstruction(pc++); //instruction fetch is also tasked with incrementing pc
        instr = decodeinstruction(cur_instruction);
        if((*instr).instr<25){
            r_instr_ptr=(struct r_type*)instr;
            execute_r(r_instr_ptr);
        } else if((*instr).instr<41){
            i_instr_ptr=(struct i_type*)instr;
            execute_i(i_instr_ptr);
        } else if((*instr).instr<43){
            j_instr_ptr=(struct j_type*)instr;
            execute_j(j_instr_ptr,pc);
        }
        //memory read/write
        //register write
        return 0;
    }
}

char* getinstruction(int pc){
    return instructions[pc];
}

struct Instruction_Type* decodeinstruction(char* instr){
    char buffer[8];
    int i=0; int instrnum; int* ptr = &i;
    struct j_type *j_instr_ptr;
    struct i_type *i_instr_ptr;
    struct r_type *r_instr_ptr;
    struct Instruction_Type* instr_ptr;
    memset(buffer,0,sizeof(buffer));
    while(*(instr+i)!=' '){
        buffer[i]=*(instr+i);
        i++;
    } 
    for(instrnum=0;instrnum<44;instrnum++){
        if(strcmp(buffer,instr_name_list[instrnum])==0){
            break;
        }
    }
    if(instrnum<25){
        r_instr_ptr=make_r_type(instrnum, instr, ptr);
        printf("inum:%d,dest:%d,s1:%d,s2:%d,shamt:%d\n",(*r_instr_ptr).instr,(*r_instr_ptr).dest_register,(*r_instr_ptr).s_register1,(*r_instr_ptr).s_register2,(*r_instr_ptr).shamt);
        instr_ptr=(struct Instruction_Type*)r_instr_ptr;
    } else if(instrnum<41){
        i_instr_ptr=make_i_type(instrnum, instr, ptr);
        printf("inum:%d,dest:%d,s:%d,imm:%d\n",(*i_instr_ptr).instr,(*i_instr_ptr).dest_register,(*i_instr_ptr).s_register,(*i_instr_ptr).immediate);
        instr_ptr=(struct Instruction_Type*)i_instr_ptr;
    } else if (instrnum<43){
        j_instr_ptr=make_j_type(instrnum, instr, ptr);
        printf("inum:%d,jump:%d\n",(*j_instr_ptr).instr,(*j_instr_ptr).jump_to);
        instr_ptr=(struct Instruction_Type*)j_instr_ptr;
    } else{
        (*instr_ptr).instr=43;
        printf("Instruction not found: %s\n", instr);
    }
    
    return instr_ptr;
}

struct exmem* execute_r(struct r_type *r_type_ptr){
    struct r_type r_instr = *r_type_ptr;
    struct exmem exmem_reg;
    struct exmem* exmem_ptr = &exmem_reg;
    memset(exmem_ptr, 0, sizeof(exmem_reg));

    exmem_reg.instrnum=r_instr.instr; exmem_reg.will_branch=0;
    if(r_instr.instr<13){ // all normal 3 register r types
        exmem_reg.alu_result = alu(r_instr.s_register1,r_instr.s_register2, r_instr.instr);
        exmem_reg.dest_register = r_instr.dest_register;
    } else if(r_instr.instr<16){ // shift operations
        exmem_reg.alu_result = alu(r_instr.s_register1,r_instr.shamt, r_instr.instr);
        exmem_reg.dest_register = r_instr.dest_register;
    } else if(r_instr.instr<22){ // mult and similar
        exmem_reg.alu_result = alu(r_instr.dest_register,r_instr.s_register1, r_instr.instr);
    } else if(r_instr.instr<24){ // mfhi and mflo
        exmem_reg.dest_register = r_instr.dest_register;
    } else{ // jr
        exmem_reg.will_branch=alu(r_instr.dest_register,0,Addi);
    }

    printf("instr:%d,alu:%d,dest:%d,branch:%d,ind:%d\n",exmem_reg.instrnum,exmem_reg.alu_result,exmem_reg.dest_register,exmem_reg.will_branch,exmem_reg.index);
    return exmem_ptr;
}

struct exmem* execute_i(struct i_type *i_type_ptr){
    struct i_type i_instr = *i_type_ptr;
    struct exmem exmem_reg;
    struct exmem* exmem_ptr = &exmem_reg;
    memset(exmem_ptr, 0, sizeof(exmem_reg));

    exmem_reg.instrnum=i_instr.instr; exmem_reg.will_branch=0;
    if(i_instr.instr==40){ //lui
        exmem_reg.alu_result=i_instr.immediate << 16;
        exmem_reg.dest_register=i_instr.dest_register;
    } else if(i_instr.instr>=37){ // all standard memory stores
        exmem_reg.alu_result=alu(i_instr.s_register, 0, Addi);
        exmem_reg.dest_register=alu(i_instr.dest_register,0,Addi);
        exmem_reg.index=i_instr.immediate;
    } else if(i_instr.instr>=32){ // all standard memory loads
        exmem_reg.alu_result=alu(i_instr.s_register, 0, Addi);
        exmem_reg.dest_register=i_instr.dest_register;
        exmem_reg.index=i_instr.immediate;
    } else if(i_instr.instr==31){ // bne
        exmem_reg.will_branch=i_instr.immediate*(alu(i_instr.s_register, i_instr.dest_register,Slt)|alu(i_instr.s_register, i_instr.dest_register,Slt));
    } else if(i_instr.instr==30){ // beq
        exmem_reg.will_branch=i_instr.immediate*(1-(alu(i_instr.s_register, i_instr.dest_register,Slt)|alu(i_instr.s_register, i_instr.dest_register,Slt)));
    } else{ //just some alu operations
        exmem_reg.alu_result=alu(i_instr.s_register, i_instr.immediate, i_instr.instr);
        exmem_reg.dest_register=i_instr.dest_register;
    }

    printf("instr:%d,alu:%d,dest:%d,branch:%d,ind:%d\n",exmem_reg.instrnum,exmem_reg.alu_result,exmem_reg.dest_register,exmem_reg.will_branch,exmem_reg.index);
    return exmem_ptr;
}

struct exmem* execute_j(struct j_type *j_type_ptr, int pc){
    struct j_type j_instr = *j_type_ptr;
    struct exmem exmem_reg;
    struct exmem* exmem_ptr = &exmem_reg;
    memset(exmem_ptr,0,sizeof(exmem_reg));

    exmem_reg.instrnum=j_instr.instr;
    if(j_instr.instr==41){ // j
        exmem_reg.will_branch=j_instr.jump_to;
    } else{ // jal
        exmem_reg.will_branch=j_instr.jump_to;
        exmem_reg.alu_result=pc; exmem_reg.dest_register=30;
    }

    printf("instr:%d,alu:%d,dest:%d,branch:%d,ind:%d\n",exmem_reg.instrnum,exmem_reg.alu_result,exmem_reg.dest_register,exmem_reg.will_branch,exmem_reg.index);
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
	else {
		perror("Command Not Found\n");
		exit(-1);
	}
	return result;
}



// decode helper functions
struct r_type* make_r_type(int instrnum, char* instr, int* ptr){
    struct r_type r_instr; 
    struct r_type *r_type_ptr=&r_instr;

    r_instr.instr=instrnum;
    r_instr.dest_register=nextregister(instr, ptr);
    if(instrnum<22){
        r_instr.s_register1=nextregister(instr, ptr);
        if(instrnum<13){
            r_instr.s_register2=nextregister(instr, ptr);
        } else if(instrnum<16){
            r_instr.shamt=(char)nextint(instr, ptr);
        }
    }

    return r_type_ptr;
}

struct i_type* make_i_type(int instrnum, char* instr, int* ptr){
    struct i_type i_instr;
    struct i_type *i_type_ptr=&i_instr;
    struct indexed_register *indreg_ptr;

    i_instr.instr=instrnum;
    i_instr.dest_register=nextregister(instr, ptr);
    if(instrnum==40){
        i_instr.immediate=nextint(instr, ptr);
    } else if(instrnum>=32){
        indreg_ptr = nextindexedregister(instr, ptr);
        i_instr.s_register=(*indreg_ptr).mem_register; i_instr.immediate=(*indreg_ptr).index;
    } else if(instrnum>=30){
        i_instr.s_register=nextregister(instr, ptr); i_instr.immediate=nextjumploc(instr,ptr);
    } else{
        i_instr.s_register=nextregister(instr, ptr); i_instr.immediate=nextint(instr, ptr);
    }

    return i_type_ptr;
}

struct j_type* make_j_type(int instrnum, char* instr, int* ptr){
    struct j_type j_instr;
    struct j_type *j_type_ptr=&j_instr;

    j_instr.instr=instrnum;
    j_instr.jump_to=nextjumploc(instr, ptr);

    return j_type_ptr;
}

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
    
    if(strcmp(reg,"$0")==0 || strcmp(reg,"$zero")==0){
        return 31;
    }
    for(newit=0;newit<31;newit++){
        if(strcmp(reg,RegisterFileNames[newit])==0){
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
    struct indexed_register indreg;
    struct indexed_register *indreg_ptr=&indreg;
    memset(reg,0,sizeof(reg));
    
    while(isspace(*(instr+i))){i++;}
    start=i;
    indreg.index = strtol(instr+start,(char**)NULL,10);
    while(*(instr+i)!='('){i++;}
    i++;
    start=i;
    while(!isspace(*(instr+i)) && *(instr+i)!=')'){
        reg[i-start]=*(instr+i);
        i++;
    }
    i = 0;
    if(strcmp(reg,"$0")==0||strcmp(reg,"$zero")==0){
        indreg.mem_register=31;
    }
    for(newit=0;newit<31;newit++){
        if(strcmp(reg,RegisterFileNames[newit])==0){
            indreg.mem_register=newit;
            i=1;
            break;
        }
    }
    if(i!=1){indreg.mem_register=32;}
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

int safe_sub(int a, int b) {
	if (a < INT_MAX && ( b < 0 && b > a)) {
		perror("Overflow Error");
		exit(-1);
	}
	else if (a > INT_MIN && b > INT_MIN + a) {
		perror("Overflow Error");
		exit(-1);
	}
	return a - b;
}

// Start of Arithmetic functions
//TODO: Mult and Divide, all operations with 'u' have to use unsigned values, others ahve to check for overflow
int add(int register2, int register3) {
	return safe_add(RegisterFile[register2], RegisterFile[register3]);
}

unsigned int addu(int register2, int register3) {
	return RegisterFile[register2] + RegisterFile[register3];
}

int sub(int register2, int register3) {
	return safe_sub(RegisterFile[register2], RegisterFile[register3]);
}

unsigned int subu(int register2, int register3) {
	return RegisterFile[register2] - RegisterFile[register3];
}

int addi(int register2, int number) {
	return safe_add(RegisterFile[register2], number);
}

unsigned int addiu(int register2, int number) {
	return RegisterFile[register2] + number;
}

void mult(int register2, int register3) {
	long long unsigned int mult1;
	long long unsigned int mult2;
	mult1 = (long long int) RegisterFile[register2];
	mult2 = (long long int) RegisterFile[register3];
	LO = ((mult1 * mult2) << 32) >> 32;
	HI = ( mult1* mult2) >> 32;
}

void multu(int register2, int register3) {
	long long unsigned int mult1;
	long long unsigned int mult2;
	mult1 = (long long unsigned int) RegisterFile[register2];
	mult2 = (long long unsigned int) RegisterFile[register3];
	LO = ((mult1 * mult2) << 32) >> 32;
	HI = (mult1* mult2) >> 32;
}

void mdiv(int register2, int register3) {
	LO = RegisterFile[register2] / RegisterFile[register3];
	HI = RegisterFile[register2] % RegisterFile[register3];
}

void divu(int register2, int register3) {
	LO = RegisterFile[register2] / RegisterFile[register3];
	HI = RegisterFile[register2] % RegisterFile[register3];
}
// Start of Logical functions

int and(int register2, int register3) {
	return (RegisterFile[register2] & RegisterFile[register3]);
}

int andi(int register2, int number) {
	return (RegisterFile[register2] & number);
}

int or(register2, register3) {
    return (RegisterFile[register2] | RegisterFile[register3]);
}

int ori(int register2, int number) {
	return (RegisterFile[register2] | number);
}

int xor(int register2, int register3) {
	return (RegisterFile[register2] ^ RegisterFile[register3]);
}

int nor(int register2, int register3) {
	return (~(RegisterFile[register2] | RegisterFile[register3]));
}

int slt(int register2, int register3) {
	if (RegisterFile[register2] < RegisterFile[register3]) {
		return 1;
	}
	else {
		return 0;
	}
}

unsigned int sltu(int register2, int register3) {
	if (RegisterFile[register2] < RegisterFile[register3]) {
		return 1;
	}
	else {
		return 0;
	}
}

int slti(int register2, int number) {
	if (RegisterFile[register2] < number) {
		return 1;
	}
	else {
		return 0;
	}
}



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
