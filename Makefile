FLAGS = -g -Wall
CC    = gcc

%.o : %.c
	$(CC) -c $^ $(FLAGS)

local: local.o common.o
	$(CC) -o $@ $^ $(FLAGS)

