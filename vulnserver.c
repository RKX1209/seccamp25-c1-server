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

void send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) break;
        sent += (size_t)n;
    }
}

static void send_text(int fd, const char *s) { send_all(fd, s, strlen(s)); }

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

        send_text(cfd, "Welcome! Please type your name:\n");
        char buf[MAX_LEN]; ssize_t n = recv(cfd, buf, sizeof(buf)-1, 0);
        if (n > 0) { buf[n] = 0; send_text(cfd, "Hello, "); send_text(cfd, buf); }
        close(cfd);
    }
    close(server_fd);
    return 0;
}

