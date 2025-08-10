// Version 4/30
// file: net_maze.c
// Build: gcc -Wall -O2 -o net_maze net_maze.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define MAX_LEN 512
#define PORT 4444

void send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) break;
        sent += (size_t)n;
    }
}

ssize_t recv_line(int fd, char *buf, size_t maxlen) {
    size_t idx = 0;
    while (idx < maxlen - 1) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n == 1) {
            if (c == '\n') break;
            buf[idx++] = c;
        } else if (n == 0) {
            break;
        } else {
            return -1;
        }
    }
    buf[idx] = '\0';
    return (ssize_t)idx;
}

int main(void){
    printf("Listening (not implemented) on %d\n", PORT);
    return 0;
}

