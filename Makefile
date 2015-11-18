# the compiler: gcc for C program, define as g++ for C++
  CC = gcc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  CFLAGS  = -g

  # the build target executable:
  TARGET = mips-in-c

  all: $(TARGET)

  $(TARGET): $(TARGET).c
		$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c -lpthread

 test1:
		$(TARGET) tests/gcd.asm > output/gcd.asm

 test2:
		$(TARGET) tests/bubble.asm > output/bubble.txt

 test3:
		$(TARGET) tests/function.asm > output/function.txt

 test4:
		$(TARGET) tests/fibonacci.asm > output/fibonacci.asm

 test: test1 test2 test3 test4

  clean:
		-rm -f *.o *.out
