##############################################
# Fall 2015, CIS 314 Computer Organization   ## Major Class Project                        ###############################################

# This tests for function calls #############
# By calculating (a*b) , (a* (b-1)), ...(a*1)
#############################################
	
# Setting the stack pointer #
	   addi $sp, $0, 256
#############################
	
	   add 	$t0, $0, $0
	   addi $t1, $0, 6    #a= 6
	   addi $t2, $0, 5    #b= 5

 	   addi $sp, $sp, -4
 	   sw 	$ra, 0($sp)

loop:	   add	$a0, $t1, $0  #Passing of the 2 arguments 
 	   add	$a1, $t2, $0
 	   jal  product
 	   add  $t3, $0, $v0  #Result return 
	   sw	$t3, 24($t0)
	   addi $t2, $t2, -1
	   addi $t0, $t0, 4
	   bne  $t2, $0, loop
	
	   lw   $ra, 0($sp)
 	   addi $sp, $sp, 4
	   j    end	   


# The function 
product:   addi	  $sp, $sp, -4
 	   sw 	  $ra, 0($sp)
	   mult   $v0, $a0, $a1
	   lw     $ra, 0($sp)
 	   addi   $sp, $sp, 4
	   jr     $ra

end:	
	