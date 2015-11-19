#include "common.h"

#define BUFFSIZE 4096

void send_method_selc_msg(int clifd, char method) {
    char buff[2] = {0x05, method};
    sendall(clifd, buff, 2);
}

struct socks5_reply {
    uint8_t ver;
    uint8_t rep;
    uint8_t rsv;
    uint8_t atyp;
    uint32_t addr;
    uint16_t port;
    uint8_t extra[2];
};

void send_reply(int clifd, uint8_t rep, uint16_t port) {
    struct socks5_reply reply;
    memset(&reply, 0, sizeof(reply));
    reply.ver = 0x05;
    reply.rep = rep;
    reply.atyp = 0x01;
    reply.port = htons(port);
    sendall(clifd, (char *) &reply, 10);
}

void parse_request(struct sockaddr_in *dst_addr, char buff[], int buff_cnt) {
    char domain[256];

    printf("buff_cnt = %d\n", buff_cnt);
    for (int i = 0; i < buff_cnt; i++) {
        printf("[%d]", (int)buff[i]);
    }
    printf("\n");

    dst_addr->sin_family = AF_INET;
    memcpy(&dst_addr->sin_port, &buff[buff_cnt-2], 2);

    switch (buff[3]) {
        case 0x01: // ipv4
            memcpy(&dst_addr->sin_addr.s_addr, &buff[4], 4);
            break;
        case 0x03: // domain
            memcpy(domain, &buff[5], buff[4]);
            domain[buff[4]] = 0;

            struct hostent *hostt = gethostbyname(domain);
            if (hostt == NULL) {
                fprintf(stderr, "gethostbyname error for host: %s: %s",
                        domain, hstrerror(h_errno));
            }

            memcpy(&dst_addr->sin_addr.s_addr, *(hostt->h_addr_list), 4);
            break;
        default:
            fprintf(stderr, "ipv6 is unsupproted now.");
    }
}

int is_complete_request(char buff[], int buff_cnt) {
    if (buff_cnt < 10) return 0;

    int len = 6;
    if (buff[3] == 0x01) { // ipv4
        len += 4;
    } else if (buff[3] == 0x03) { // domain name
        len += 1 + buff[4];
    } else if (buff[3] == 0x04) { // ipv6
        len = 16;
    }
    
    // printf("buff_cnt should is %d, actual buff_cnt is %d\n", len, buff_cnt);
    return buff_cnt >= len;
}

void socks5_server(int clifd) {
    char buff[BUFFSIZE];
    int buff_cnt = 0, n;

    while (1) {
        n = recv(clifd, buff + buff_cnt, BUFFSIZE - buff_cnt, 0);
        if (n < 0) {
            syslog(LOG_ERR, "%s", strerror(errno));
            close(clifd);
            exit(-1);
        }
        buff_cnt += n;

        if (buff_cnt >= 1 && buff[0] != 0x05) { 
            // syslog(LOG_INFO, "unsupported protocol version.");
            printf("unsupported protocol version.\n");
            send_method_selc_msg(clifd, 0xFF);
            close(clifd);
            exit(-1);
        }
        if (buff_cnt >= 2 && buff_cnt - 2 >= buff[1]) {
            printf("pass authentication.\n");
            send_method_selc_msg(clifd, 0x00);
            break;
        }
    }
    

    struct sockaddr_in dst_addr;
    memset(&dst_addr, 0, sizeof(dst_addr));

    buff_cnt = 0;
    while (1) {
        n = recv(clifd, buff + buff_cnt, BUFFSIZE - buff_cnt, 0);
        if (n <= 0) {
            // syslog(LOG_ERR, "%s", strerror(errno));
            printf("%s", strerror(errno));
            close(clifd);
            exit(-1);
        }
        buff_cnt += n;

        if (is_complete_request(buff, buff_cnt)) {
            if (buff[1] != 0x01) { // CMD is not CONNECT
                // syslog(LOG_INFO, "command not supported");
                printf("command not supported");
                send_reply(clifd, 0x07, 0);
                exit(-1);
            }
            parse_request(&dst_addr, buff, buff_cnt);
            break;
        }
    }

    printf("destition is %s:%d\n",
           inet_ntop(AF_INET, &dst_addr.sin_addr, buff, sizeof(buff)),
           ntohs(dst_addr.sin_port));

    int dstfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(dstfd, (struct sockaddr *) &dst_addr, sizeof(dst_addr)) < 0) {
        send_reply(clifd, 0x05, 0); // connection refused
        // syslog(LOG_ERR, "%s", strerror(errno));
        printf("%s\n", strerror(errno));
        exit(-1);
    }
    send_reply(clifd, 0x00, dst_addr.sin_port);

    /*
    forward(clifd, dstfd);
    forward(dstfd, clifd);

    close(clifd);
    close(dstfd);
    */

    bd_forword(clifd, dstfd);
}

int main(int argc, const char *argv[]) {
    // struct socks5_reply sr;
    // printf("socks5_reply len = %d\n", sizeof(sr));
    if (argc != 2) {
        printf("usage: %s <server_port>\n", argv[0]);
        exit(-1);
    }
    
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // syslog(LOG_ERR, "%s\n", strerror(errno));
        printf("%s\n", strerror(errno));
        exit(-1);
    }
    int sock_reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_reuse, sizeof(sock_reuse));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
        // syslog(LOG_ERR, "%s", strerror(errno));
        printf("%s\n", strerror(errno));
        exit(-1);
    }
    listen(listenfd, 5);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t cli_len = sizeof(cliaddr);
        int connfd = accept(listenfd, (struct sockaddr*) &cliaddr, &cli_len);
        if (connfd < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            continue;
        }
        char buff[100];
        printf("connect from %s:%d\n",
               inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)),
               ntohs(cliaddr.sin_port));

        pid_t child_pid;
        if ((child_pid = fork()) == 0) { // child process
            close(listenfd);
            socks5_server(connfd);
            exit(0);
        } else { // parent process
            close(connfd);
        }
    }
    return 0;
}

