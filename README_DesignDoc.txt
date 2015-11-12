/* 
*  Design of the Single-Cycle Process
*  Project by Matthew Jagielski, Jacob Bieker, and Theodore LaGrow
*  Date Submitted: 11/11/2015
*  Class: CIS 314
*/ 

General notes:

We decided to lay out this program looking forward to the pipelining implementation next week.  We decided to put the helper functions of all of pipeline stages at the end of the code to help with scrolling in the program.  The code in in one script to help with merging with github (probably not the best design, but this worked for us).  For our simulator, we wanted to have ALL of the real integer instructions (taken from Wikipedia). The layout below is as follows with the flow of the code:


----------------------------------------------------------------------

headers 
	- Initializes all of the variables, array, and functions that will be used in the code.
	- We created struct types for all of the different types of instructions, the index register, and for both execute memory and memory writeback. 
	- Matthew has added comments with his struc headers to indicate how the pointers operate and function with the design of the program.

main
	- The main takes in the assembly file and runs the file through our program.
	- We used the main for rigorous amount of testing as well.

readmips
	- The function has the appropriate flags to check to see if the instruction is valid.  
	- The function reads in the MIPS instructions and accounts for the differing amount of elements an instruction can have.

control logic
	- The printstate() function helps with the front end of the project to actually see what is being run by the program.
	- The control logic checks the type of instruction being sent.  
	- The instructions are also sent to the appropriate pipelining stage that it needs to go to next.

fetch
	- getinstruction() gets the instruction (as indicated in main() ) and puts the data into the instruction[] array for control logic to operate on.

decode
	- The decodeinstruction() is taking in the instruction after fetch and (depending on its type) will parse through and make the correct values within the structs to delineate in pieces what the instruction wants. 

execute 
	- The pipeline stage consists of three excuse for the three types of instruction type.
		- The execute functions will create an appropriate exmem struct that will create a destination where the values can be store so the memory pipeline stage can use those values.
		- Each function will return a stuct that will take in it's type from the decode stage
		- The execute_r() passes all of the instructions with 3 registers indicated by the instruction, the shift operations, the mult and div instructions, mfhi and mflo, and jr instruction.
		- The execute_i() passes the lui instruction, all of the standard memory stores, all of the standard memory loads, the bne instruction, beq instruction, and then the immediate alu instructions (addi, etc.).
		- The execute_j function accounts for the j instruction, and the jal instruction. 
	- The ALU is stationed betweeen execute and memory 
		- All of the if/else statements that have to do with the logical operators.

memory
	- The memory_rw() function takes in the specific instruction from decode.  The function will then create an appropriate memwb struct to allocate a destination for the values produced by the function.

writeback
	- The writeback() function produces the result and gives the value to the user
	- This function also takes into account the mfhi, mflo, and mtzZ

fetch helpers
	- The functions make_r_type(), make_i_type(), and make_j_type() are functions that take in the instructions

decode helpers
	- Functions that helps the Decode pipeline are located here.

execute helpers
	- All of the Arithmetic Logical functions and shifting functions are here.

memory helpers
	- All of the Data Transfer load functions are here.

writeback helpers
	- All of the Data Transfer store functions are here.

america()
	- This function uses the free() method to release the memory we have malloc'ed with the pointers and structs. 

