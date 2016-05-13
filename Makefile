# the compiler: gcc for C program, define as g++ for C++
C = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -std=gnu99 -Wall -Wextra 


DOXYGEN = doxygen

# the build target executable:

default: server client

server: server.c
client: client.c

docs:
	$(DOXYGEN) Doxyfile 

clean:
	$(RM) -rf server client login Doxyfile.bak html latex
