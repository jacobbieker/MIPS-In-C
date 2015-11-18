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

  clean:
  	$(RM) $(TARGET)
