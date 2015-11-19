FLAGS = -g -Wall
CC    = gcc

%.o : %.c %.h
	$(CC) -c $^ $(FLAGS)

local: local.o common.o
	$(CC) -o $@ $^ $(FLAGS)

