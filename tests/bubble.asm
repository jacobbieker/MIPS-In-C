##############################################
# Fall 2015, CIS 314 Computer Organization   #
# Major Class Project                        #
##############################################

# MIPS: Bubble Sort
sub $t0, $t0, $t0          # clear the content of $t0
addi $t7, $t0, 20          # starting address of the array 
addi $t1, $t0, 4 
addi $t2, $t0, 5 
addi $t3, $t0, 10 
addi $t4, $t0, 2 
addi $t5, $t0, 7
addi $t6, $t0, 1 
sw   $t1, 0($t7)
sw   $t2, 4($t7)
sw   $t3, 8($t7)
sw   $t4, 12($t7)
sw   $t5, 16($t7)
sw   $t6, 20($t7)
addi $t1, $t0, 6 
addi $t2, $t0, 3 
addi $t3, $t0, 17 
addi $t4, $t0, 9
addi $t5, $t0, 5
sw   $t1, 24($t7)
sw   $t2, 28($t7)
sw   $t3, 32($t7)
sw   $t4, 36($t7)
sw   $t5, 40($t7)
main: addi $t0, $0, 10      # number of elements -1 in the array
	  add $t1, $t7, $0
	  add $t2, $0, $0
loop: beq $t0,$zero,done
	  lw $t6,0($t1)         # $t6 = *(pa)
	  lw $t5,4($t1)         # $t5 = *(pa+1)
	  sltu $t3,$t6,$t5      # if ($t6 < $t5)
	  beq $t3,$zero,next    # goto next
	  sw $t6,4($t1)         #
	  sw $t5,0($t1)         # *pa <-> *(pa+1)
	  addi $t2, $0,1        # set exchange flag
next: addi $t1,$t1,4        # pa++
	  addi $t0,$t0,-1       # $t0 = $t0 -1
      j loop
done: bne $t2,$zero,main    # if (exchange) goto main

end: 

