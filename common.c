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

