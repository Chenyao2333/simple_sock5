#include "common.h"

#define BUFFSIZE 4096
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

int recv_buff(int fd, char buff[], int *buff_cnt) {
    int n = enc_recv(fd, buff + (*buff_cnt), BUFFSIZE - (*buff_cnt), 0);
    if (n >= 0) (*buff_cnt) += n;
    fprintf(stdout, "recv %d bytes from fd=%d\n", n, fd);
    return n;
}

int send_buff(int fd, char buff[], int *buff_cnt) {
    int n = send(fd, buff, *buff_cnt, 0);
    if (n >= 0) {
        memcpy(buff, buff + n, (*buff_cnt) - n);
        (*buff_cnt) -= n;
    }
    fprintf(stdout, "send %d bytes to fd=%d\n", n, fd);
    return n;
}

void bd_forword(int fd1, int fd2) {
    fd_set readset, writeset;
    int max_fd = (fd1 > fd2 ? fd1 : fd2) + 1;

    set_non_blocking(fd1);
    set_non_blocking(fd2);

    char buff1[BUFFSIZE], buff2[BUFFSIZE];
    int buff1_cnt = 0, buff2_cnt = 0;

    while (1) {
        FD_ZERO(&readset);
        FD_ZERO(&writeset);

        if (buff1_cnt < BUFFSIZE) {
            FD_SET(fd1, &readset);
        }
        if (buff2_cnt < BUFFSIZE) {
            FD_SET(fd2, &readset);
        }
        if (buff1_cnt) {
            FD_SET(fd2, &writeset);
        }
        if (buff2_cnt) {
            FD_SET(fd1, &writeset);
        }

        int n = select(max_fd, &readset, &writeset, NULL, NULL);
        if (n < 0) {
            fprintf(stderr, "bd_forword: %s\n", strerror(errno));
            close(fd1); close(fd2);
            pthread_exit(NULL);
        }

        if (FD_ISSET(fd1, &readset)) {
            if (recv_buff(fd1, buff1, &buff1_cnt) <= 0) goto ret;
        }
        if (FD_ISSET(fd2, &readset)) {
            if (recv_buff(fd2, buff2, &buff2_cnt) <= 0) goto ret;
        }
        if (FD_ISSET(fd1, &writeset)) {
            if (send_buff(fd1, buff2, &buff2_cnt) <= 0) goto ret;
        }
        if (FD_ISSET(fd2, &writeset)) {
            if (send_buff(fd2, buff1, &buff1_cnt) <= 0) goto ret;
        }
    }
ret:
    fprintf(stdout, "forword be closed, err: %s\n", strerror(errno));
    close(fd1);
    close(fd2);
    pthread_exit(NULL);
}
