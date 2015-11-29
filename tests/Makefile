CC     = gcc
FLAGS  = -g -Wall -std=c99 -pthread

SOURCES = utils.c
HEADERS = test.h utils.h

all: test_local test_server

test_local: test_local.c $(SOURCES) $(HEADERS)
	$(CC) $(FLAGS) $(SOURCES) test_local.c -o $@

test_server: test_server.c $(SOURCES) $(HEADERS)
	$(CC) $(FLAGS) $(SOURCES) test_server.c -o $@
