#ifndef _H_COMMON
#define _H_COMMON

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <netdb.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void sendall(int fd, char buff[], int buff_size);
void forward(int srcfd, int dstfd);

#endif // #ifndef _H_COMMON
