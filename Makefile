CC    = gcc
FLAGS = -g -Wall -std=c99 -pthread

REMOTE_FLAG = -DIS_REMOTE
LOCAL_FLAG  = -DIS_LOCAL

HEADERS = socks5.h common.h
SOURCES  = socks5.c common.c

all: local remote

local: $(SOURCES) $(HEADERS)
	$(CC) $(FLAGS) $(LOCAL_FLAG) $(SOURCES) -o $@

remote:$(SOURCES) $(HEADERS)
	$(CC) $(FLAGS) $(REMOTE_FLAG) $(SOURCES) -o $@
