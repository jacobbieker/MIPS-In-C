/* 
*  Design of the Pipelining Implimentation
*  Project by Matthew Jagielski, Jacob Bieker, and Theodore LaGrow
*  Date Submitted: 11/18/2015
*  Class: CIS 314
*/ 

General notes:

Our program is relativly the same as last week.  We set ourselves up well for this week creating structs and functions to impliment the five processing stages.  We decided to conduct the pipelining with the pthreading module.  Here is how we modified the code from last week:

---------------------------------------------------------------------------

- We began by changing our excute stage from having three fuctions that accounted for all three types of MIPS intructions to having one centeral execute function that will help with the control flow of the stage.  This way we would be able to deal with the pthreading with only function with execute.

- There was an error that arose when the execute function was combined that outputted zeros for all of our values.  Matthew found the fix.  What happened was that space was being malloc'd when it was also trying to be written on and the output was zero.  Our execute function is now working.

- We made each central staged function (getinstructions, decodeinstruction, execute, memory_rw, writeback) a void function and made the functions return a void*.  This way the functions fit into the parameters of the pthreading so we could operate pthreading.

- We added 5 pthread_creates for each stage in our control logic function

- We added 5 pthread_join for each stage in control logic.  This will make sure each thread will wait before starting the next clock cycle (what we've been calling another round of processing) so no function is called multiple (or incorrect) amount of times.

- We added 5 structs to take in account for when a stage completes and needs to write to the next cycle but the next cycle is still running.  We called these the intermediary structs.

- Jacob made a Makefile to help with compliling our program (there is an extra piece that needs to be added for the pthreading [-lpthread]).  The Makefile is also set up to run all of the tests so we don't have to iterate through each one.

- (HAAAZZZZARRRRDS)