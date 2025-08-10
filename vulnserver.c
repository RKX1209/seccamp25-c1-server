// file: net_maze.c
// Build: gcc -Wall -O2 -o net_maze net_maze.c
// Run:   ./net_maze
// Connect: nc 127.0.0.1 1234

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_LEN 512
#define PORT 1234
#define NAME_MAX 32

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
        char c; ssize_t n = recv(fd, &c, 1, 0);
        if (n == 1) {
            if (c == '\r') continue;
            if (c == '\n') break;
            buf[idx++] = c;
        } else if (n == 0) break;
        else return -1;
    }
    buf[idx] = '\0';
    return (ssize_t)idx;
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; addr.sin_port = htons(PORT); addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); close(server_fd); return 1; }
    if (listen(server_fd, 4) < 0) { perror("listen"); close(server_fd); return 1; }
    printf("Listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in caddr; socklen_t clen = sizeof(caddr);
        int cfd = accept(server_fd, (struct sockaddr*)&caddr, &clen);
        if (cfd < 0) { perror("accept"); continue; }

        const char *welcome_msg = "Welcome! Send your player name (max 31 chars):\n";
        send_all(cfd, welcome_msg, strlen(welcome_msg));

        char linebuf[MAX_LEN];
        ssize_t n = recv_line(cfd, linebuf, sizeof(linebuf));
        if (n > 0) {
            char name[NAME_MAX];
            strncpy(name, linebuf, NAME_MAX-1); name[NAME_MAX-1] = 0;
            char out[128];
            snprintf(out, sizeof(out), "Hello, %s\n", name);
            send_all(cfd, out, strlen(out));
        }
        close(cfd);
    }
    close(server_fd);
    return 0;
}

