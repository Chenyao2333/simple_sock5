#include "common.h"

void sendall(int fd, char buff[], int buff_size) {
    int buff_cnt = 0, n;
    while (buff_cnt < buff_size) {
        if ((n = send(fd, buff + buff_cnt, buff_size - buff_cnt, 0)) < 0) {
            syslog(LOG_ERR, "%s", strerror(errno));
        }
        buff_cnt += n;
        printf("success send %d byte.\n", n);
    }
}

void forward(int srcfd, int dstfd) {
    pid_t child_pid= fork();
    if (child_pid != 0) {
        return ;
    }

    char buff[4096];
    int n;
    while (1) {
        if ((n = recv(srcfd, buff, 4096, 0)) <= 0) {
            syslog(LOG_ERR, "%s", strerror(errno));
            close(srcfd);
            close(dstfd);
            exit(0);
        }
        buff[n] = 0;
        // printf("recv:\n%s\n", buff);

        sendall(dstfd, buff, n);
    }
}

void recv_send(int fd1, int fd2) {
    #define BUFFSIZE 4096
    char buff[BUFFSIZE];
    int n = recv(fd1, buff, BUFFSIZE, 0);
    if (n == 0) {
        fprintf(stderr, "closed connection.\n");
        exit(0);
    }
    printf("recv %d bytes.\n", n);
    if (send(fd2, buff, n, 0) != n) {
        fprintf(stderr, "test!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!can not send all data in one time!\n");
        exit(-1);
    }
}
void bd_forword(int fd1, int fd2) {
    fd_set readset;
    int max_fd = (fd1 > fd2 ? fd1 : fd2) + 1;
    int on = 1;

    ioctl(fd1, FIONBIO, (char *) &on);
    ioctl(fd2, FIONBIO, (char *) &on);

    while (1) {
        FD_ZERO(&readset);
        FD_SET(fd1, &readset);
        FD_SET(fd2, &readset);

        int n = select(max_fd, &readset, NULL, NULL, NULL);
        if (n < 0) {
            fprintf(stderr, "bd_forword: %s\n", strerror(errno));
            exit(-1);
        }

        if (FD_ISSET(fd1, &readset)) {
            recv_send(fd1, fd2);
        }
        if (FD_ISSET(fd2, &readset)) {
            recv_send(fd2, fd1);
        }
    }
}
