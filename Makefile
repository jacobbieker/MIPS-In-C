# the compiler: gcc for C program, define as g++ for C++
  CC = gcc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  CFLAGS  = -g

  # the build target executable:
  TARGET = mips-in-c

  all: $(TARGET)

  $(TARGET): $(TARGET).c
  	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

  test1:
		$(TARGET) < test/gcd.asm

	test2:
		$(TARGET) < test/bubble.asm

	test3:
		$(TARGET) < function.asm

	test4:
		$(TARGET) < fibonacci.asm

	test: test1 test2 test3 test4
	
  clean:
  	-rm -f *.o *.out
