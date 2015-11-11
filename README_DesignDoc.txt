Design of the Single-Cycle Process
Project by Matthew Jagielski, Jacob Bieker, and Theodore LaGrow
Date 11/11/2015
CIS 314


We decided to lay out this program looking forward to the pipelining implimintation next week.  We decided to put the helper functions of all of pipeline stages at the end of the code to help with scrolling in the program.  The code in in one script to help with merging with github (probably not the best design, but this worked for us).  The layout below is as follow with the flow of the code:


headers 
	- Initalizes all of the variables, array, and functions that will be used in the code.

main

readmips

control logic

fetch

decode

execute (with alu)

memory

wb

fetch helpers

decode helpers

execute helpers

memory helpers

wb helpers

america()
