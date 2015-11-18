##############################################
# Fall 2015, CIS 314 Computer Organization   #
# Major Class Project                        #
##############################################

# Greatest Common Divider 
#	int gcd (int a, int b)
#	{
#		while (a != b){ 
#			if (a > b) a-=b; 
#			else b-= a;
#		}
#		return a; 
#	}

#  gcd (93, 62)
	   sub $t0, $t0, $t0  # clear the content of $t0
	   addi $t1, $t0, 93  # load a = 93 into $t1
	   addi $t2, $t0, 62  # load b = 62 into $t2 
	   sw   $t1, 4($t0)
	   sw   $t2, 8($t0)
Again: beq  $t1, $t2, Done
       sltu $t3,$t1,$t2
       bne  $t3,$zero, ALess
       sub  $t1, $t1,$t2
       j    Again
ALess: sub  $t2, $t2,$t1
       j    Again
Done:  sw   $t1, 16($t0)  # storing the answer
	   sw   $t2, 32($t0)
#  END 