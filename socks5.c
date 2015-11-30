#include "socks5.h"

#define BUFFSIZE 4096

#ifdef IS_REMOTE
void send_method_selc_msg(int clifd, char method) {
    char buff[2] = {0x05, method};
    enc_sendall(clifd, buff, 2);
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
    reply.port = port;
    enc_sendall(clifd, (char *) &reply, 10);
}

int parse_request(struct sockaddr_in *dst_addr, char buff[], int buff_cnt) {
    int i;
    char domain[256+10];

    /*
    printf("buff_cnt = %d\n", buff_cnt);
    for (int i = 0; i < buff_cnt; i++) {
        printf("[%d]", (int)buff[i]);
    }
    printf("\n");
    */

    fprintf(stdout, "buff_cnt = %d\n", buff_cnt);
    for (i = 0; i < buff_cnt; i++) {
        fprintf(stdout, "%d ", (int)buff[i]);
    }
    fprintf(stdout, "\n");

    dst_addr->sin_family = AF_INET;
    memcpy(&dst_addr->sin_port, &buff[buff_cnt-2], 2);

    switch (buff[3]) {
        case 0x01: // ipv4
            memcpy(&dst_addr->sin_addr.s_addr, &buff[4], 4);
            break;
        case 0x03: // domain
            memcpy(domain, &buff[5], buff[4]);
            domain[buff[4]] = 0;
            fprintf(stdout, "request domain is: %s\n", domain);

            struct hostent *hostt = gethostbyname(domain);
            if (hostt == NULL) {
                // I don't know why follow statement will cause Segmentitaion fault.
                // fprintf(stderr, "gethostbyname error for host: %s: %s\n",
                //         domain, hstrerror(h_errno));
                return -1;
                break;
            }

            if (*(hostt->h_addr_list)) {
                memcpy(&dst_addr->sin_addr.s_addr, *(hostt->h_addr_list), 4);
            } else {
                fprintf(stderr,"interasting! %s doesn't have A record!\n", domain);
                return -1;
            }
            break;
    }
    return 0;
}

int is_complete_request(char buff[], int buff_cnt) {
    if (buff_cnt < 10) return 0;

    int len = 6;
    if (buff[3] == 0x01) { // ipv4
        len += 4;
    } else if (buff[3] == 0x03) { // domain name
        len += 1 + buff[4];
    } else if (buff[3] == 0x04) { // ipv6
        len += 16;
    }
    
    // printf("buff_cnt should is %d, actual buff_cnt is %d\n", len, buff_cnt);
    return buff_cnt >= len;
}

void *socks5_server(void *clifd_p) {
    int clifd = *((int *)clifd_p);
    free(clifd_p);
    char buff[BUFFSIZE];
    int buff_cnt = 0, n;

    while (1) {
        n = enc_recv(clifd, buff + buff_cnt, BUFFSIZE - buff_cnt, 0);
        if (n <= 0) {
            if (n < 0) fprintf(stderr, "recv authentication info: %s\n", strerror(errno));
            close(clifd);
            pthread_exit(NULL);
        }
        buff_cnt += n;

        if (buff_cnt >= 1 && buff[0] != 0x05) { 
            // syslog(LOG_INFO, "unsupported protocol version.");
            printf("unsupported protocol version.");
            send_method_selc_msg(clifd, 0xFF);
            close(clifd);
            pthread_exit(NULL);
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
        n = enc_recv(clifd, buff + buff_cnt, BUFFSIZE - buff_cnt, 0);
        if (n <= 0) {
            if (n < 0) fprintf(stderr, "recv request: %s\n", strerror(errno));
            close(clifd);
            pthread_exit(NULL);
        }
        buff_cnt += n;

        if (is_complete_request(buff, buff_cnt)) {
            if (buff[1] != 0x01) { // CMD is not CONNECT
                fprintf(stderr, "command not supported\n");
                send_reply(clifd, 0x07, 0);
                close(clifd);
                pthread_exit(NULL);
            }
            if (parse_request(&dst_addr, buff, buff_cnt) < 0) {
                fprintf(stderr, "perhaps occurs some error at query dns");
                send_reply(clifd, 0xFF, 0);
                close(clifd);
                pthread_exit(NULL);
            }
            break;
        }
    }

    printf("clifd: %d destition is %s:%d\n",
           clifd,
           inet_ntop(AF_INET, &dst_addr.sin_addr, buff, sizeof(buff)),
           ntohs(dst_addr.sin_port));

    int dstfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(dstfd, (struct sockaddr *) &dst_addr, sizeof(dst_addr)) < 0) {
        send_reply(clifd, 0x05, 0); // connection refused
        fprintf(stderr, "connect: %s\n", strerror(errno));
        close(clifd); close(dstfd);
        pthread_exit(NULL);
    }
    send_reply(clifd, 0x00, dst_addr.sin_port);

    bd_forword(clifd, dstfd);
}
#endif

#ifdef IS_LOCAL
struct sockaddr_in remote_addr;
void *relay(void *p_clifd) {
    int clifd = *((int *)p_clifd);
    free(p_clifd);

    struct sockaddr_in dst_addr;
    memcpy(&dst_addr, &remote_addr, sizeof(dst_addr));
    char buff[32];
    printf("remote address: %s:%d\n",
           inet_ntop(AF_INET, &dst_addr.sin_addr, buff, sizeof(buff)),
           ntohs(dst_addr.sin_port));
    int dstfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(dstfd, (struct sockaddr *) &dst_addr, sizeof(dst_addr)) < 0) {
        fprintf(stderr, "conot connect to remote server: %s\n", strerror(errno));
        close(clifd);
        pthread_exit(NULL);
    }

    bd_forword(clifd, dstfd);
}
#endif

int main(int argc, const char *argv[]) {
# ifdef IS_REMOTE
    if (argc != 2) {
        fprintf(stderr, "usage: %s <server_port>\n", argv[0]);
        exit(1);
    }
# else
    if (argc != 4) {
        fprintf(stderr, "usage: %s <local_port> <server_address> <server_port>\n", argv[0]);
        exit(1);
    }

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[2], &remote_addr.sin_addr);
    remote_addr.sin_port = htons(atoi(argv[3]));
# endif
    
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stdout, "creat socket: %s\n", strerror(errno));
        exit(1);
    }

    int sock_reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_reuse, sizeof(sock_reuse));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    char buff[32];

    if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "bind: %s\n", strerror(errno));
        exit(1);
    }
    listen(listenfd, 1024);
    fprintf(stdout, "service on %s:%d\n",
            inet_ntop(AF_INET, &servaddr.sin_addr, buff, sizeof(buff)),
            ntohs(servaddr.sin_port));

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t cli_len = sizeof(cliaddr);
        int connfd = accept(listenfd, (struct sockaddr*) &cliaddr, &cli_len);
        if (connfd < 0) {
            fprintf(stderr, "accept: %s\n", strerror(errno));
            continue;
        }
        fprintf(stdout, "clifd: %d connect from %s:%d\n",
                connfd,
                inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)),
                ntohs(cliaddr.sin_port));

        pthread_t tid;
        int *p_connfd = malloc(sizeof(*p_connfd));
        *p_connfd = connfd; // After this loop ending, the &connfd is be released, so cannot directly pass &connfd as args!!! It's spend me 3 hours!
#ifdef IS_REMOTE
        errno = pthread_create(&tid, NULL, socks5_server, p_connfd);
#else
        errno = pthread_create(&tid, NULL, relay, p_connfd);
#endif
        if (errno) {
            fprintf(stderr, "pthread_create: %s\n", strerror(errno));
        }
    }
    return 0;
}

