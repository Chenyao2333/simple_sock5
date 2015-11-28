#ifndef _H_COMMON
#define _H_COMMON

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <netdb.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

int set_non_blocking(int fd);
int enc_recv(int fd, char *buff, int buff_size, int flags);

void enc_sendall(int fd, char buff[], int buff_size);
void bd_forword(int srcfd, int dstfd);

#endif // _H_COMMON
