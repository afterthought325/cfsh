objects = main.o
CC = gcc -std=c11

all: cfsh

debug: CCFLAGS += -Wall -Wextra -DDEBUG -g
debug: all

cfsh: $(objects)
	$(CC) -o cfsh $(objects)

main.o:
	$(CC) $(CCFLAGS) -c main.c

.PHONY : clean
clean: 
	-rm cfsh $(objects)
