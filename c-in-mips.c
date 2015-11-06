#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// The first and (probably) only comment.... :DDD

static int RegisterFile[31] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char* RegisterFileNames[31] = {'$v0', '$v1', '$a0', '$a1', '$a2', '$a3', '$t0', '$t1', '$t2', '$t3', '$t4', '$t5', '$t6', '$t7', '$s0', '$s1', '$s2', '$s3', '$s4', '$s5', '$s6', '$s7', '$t8', '$t9', '$gp', '$sp', '$fp', '$ra'};
static int data_memory[1024];
static char* string_memory[2014];
static char* instructions[1000000];

int main() {


}

