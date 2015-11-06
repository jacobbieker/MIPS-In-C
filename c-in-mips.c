#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

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
    if (assembly == 0)
    {
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
