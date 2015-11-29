#include "test.h"
#include "utils.h"

#define BUFFSIZE 4096

void recv_data(int cli_fd) {
    int i;
    char buff[BUFFSIZE];
    if (recv(cli_fd, buff, 4, 0) != 4) {
        perror("recv");
        exit(1);
    }
    uint32_t total_length;
    memcpy(&total_length, buff, 4);
    total_length = ntohl(total_length);

    uint8_t last_byte = 0;
    int recv_len = 0;
    while (recv_len < total_length) {
        int n;
        if ((n = recv(cli_fd, buff, BUFFSIZE, 0)) < 0) {
            perror("recv");
            exit(1);
        }
        if (n == 0) break;
        for (i = 0; i < n; i++) {
            last_byte = generate_next_byte(last_byte, recv_len, 0);
            if (last_byte != (uint8_t)buff[i]) {
                fprintf(stdout, "has recived %d bytes\n", recv_len);
                perror("test: error byte!!!!!!!!!");
                exit(1);
            }
            recv_len++;
        }
    }

    if (recv_len != total_length) {
        perror("test: recv_length doesn't correspond total_length.");
        exit(1);
    }
    printf("test: pass test!\n");
}

int main(int argc, const char *argv[]) {
    struct sockaddr_in serv_addr;
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons(12890);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int listen_fd;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse));

    if (bind(listen_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    listen(listen_fd, 1024);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int conn_fd = accept(listen_fd, (struct sockaddr*) &cli_addr, &cli_len);
        if (conn_fd < 0) {
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(conn_fd);
        } else if(pid > 0) {
            close(listen_fd);
            recv_data(conn_fd);
            exit(0);
        } else {
            perror("fork");
        }
    }
    return 0;
}
