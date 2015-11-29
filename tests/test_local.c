#include "test.h"
#include "utils.h"

#define BUFFSIZE 4096
#define TOTAL_LENGTH 1024*5
#define THREAD_NUM 20

char socks5_address[] = "127.0.0.1";
uint16_t socks5_port = 1080;

char dst_address[] = "127.0.0.1";
uint16_t dst_port = 12890;


void test_socks5() {
    int i, n;
    char buff[BUFFSIZE];
    struct sockaddr_in socks5;
    CL(socks5);
    socks5.sin_family = AF_INET;
    socks5.sin_port   = htons(socks5_port);
    inet_pton(AF_INET, socks5_address, &socks5.sin_addr);

    fprintf(stdout, "socks5 server: %s:%d\n",
            inet_ntop(AF_INET, &socks5.sin_addr, buff, sizeof(buff)),
            ntohs(socks5.sin_port));

    int socks5_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(socks5_fd, (struct sockaddr *) &socks5, sizeof(socks5)) < 0) {
        perror("conect to socks5 server");
        pthread_exit(NULL);
    }

    char handshark[] = {0x05, 0x01, 0x00};
    char request[300] = {0x05, 0x01, 0x00, 0x03};
    int request_len = 4;

    request[request_len++] = sizeof(dst_address);
    memcpy(&request[request_len], dst_address, sizeof(dst_address));
    request_len += sizeof(dst_address);
    uint16_t tmp = htons(dst_port);
    memcpy(&request[request_len], &tmp, 2);
    request_len += 2;

    send(socks5_fd, handshark, 3, 0);
    if ((n = recv(socks5_fd, buff, BUFFSIZE, 0)) != 2) {
        perror("recv auth info");
        pthread_exit(NULL);
    }

    send(socks5_fd, request, request_len, 0);
    if ((n = recv(socks5_fd, buff, BUFFSIZE, 0)) != 10) { // just consider length is 10
        fprintf(stderr, "recv %d byte for reply\n", n);
        perror("recv reply");
        pthread_exit(NULL);
    }

    uint32_t total_length = TOTAL_LENGTH;
    uint32_t n_tmp = htonl(total_length);
    memcpy(buff, &n_tmp, 4);
    send(socks5_fd, buff, 4, 0);

    uint8_t last_byte = 0;
    int buff_cnt = 0, sended = 0;
    while (sended < total_length) {
        buff_cnt = 0;
        while (buff_cnt < BUFFSIZE && sended + buff_cnt < total_length) {
            last_byte = generate_next_byte(last_byte, sended + buff_cnt, 0);
            buff[buff_cnt++] = last_byte;
        }
        if ((n = send(socks5_fd, buff, buff_cnt, 0)) != buff_cnt) {
            fprintf(stdout, "send %d bytes\n", n);
            perror("has not send all data in one time call of send");
            pthread_exit(NULL);
        }
        sended += buff_cnt;
    }

}

int main(int argc, const char *argv[]) {
    int i;

    for (i = 0; i < THREAD_NUM; i++) {
        pthread_t tid;
        pthread_create(&tid, NULL, &test_socks5, NULL);
    }
    printf("creat thread end\n");

    while (1) {
        sleep(1);
    }
    return 0;
}
