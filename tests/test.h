#ifndef _H_TEST
#define _H_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#define CL(x) memset(&x, 0, sizeof(x))

#endif // _H_TEST
