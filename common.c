#include "common.h"

#define BUFFSIZE 1024
#define XOR_ENC_KEY 169
int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return flags;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int enc_recv(int fd, char *buff, int buff_size, int flags) {
    int n = recv(fd, buff, buff_size, flags);
    for (int i = 0; i < n; i++) {
        buff[i] ^= XOR_ENC_KEY;
    }
    return n;
}


void enc_sendall(int fd, char buff[], int buff_size) {
    int buff_cnt = 0, n;
    for (int i = 0; i < buff_size; i++) {
        buff[i] ^= XOR_ENC_KEY;
    }
    while (buff_cnt < buff_size) {
        if ((n = send(fd, buff + buff_cnt, buff_size - buff_cnt, 0)) < 0) {
            syslog(LOG_ERR, "%s", strerror(errno));
        }
        buff_cnt += n;
        // printf("success send %d byte.\n", n);
    }
}

void recv_send(int fd1, int fd2) {
    char buff[BUFFSIZE];
    int n = enc_recv(fd1, buff, BUFFSIZE, 0);
    if (n == 0) {
        fprintf(stderr, "closed connection.\n");
        close(fd1); close(fd2);
        pthread_exit(NULL);
    }
    // printf("recv %d bytes.\n", n);
    if (send(fd2, buff, n, 0) != n) {
        fprintf(stderr, "test!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!can not send all data in one time!\n");
        close(fd1); close(fd2);
        pthread_exit(NULL);
    }
}
void bd_forword(int fd1, int fd2) {
    fd_set readset;
    int max_fd = (fd1 > fd2 ? fd1 : fd2) + 1;

    set_non_blocking(fd1);
    set_non_blocking(fd2);

    while (1) {
        FD_ZERO(&readset);
        FD_SET(fd1, &readset);
        FD_SET(fd2, &readset);

        int n = select(max_fd, &readset, NULL, NULL, NULL);
        if (n < 0) {
            fprintf(stderr, "bd_forword: %s\n", strerror(errno));
            close(fd1); close(fd2);
            pthread_exit(NULL);
        }

        if (FD_ISSET(fd1, &readset)) {
            recv_send(fd1, fd2);
        }
        if (FD_ISSET(fd2, &readset)) {
            recv_send(fd2, fd1);
        }
    }
}
