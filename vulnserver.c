// file: net_maze.c
// Build: gcc -Wall -O2 -o net_maze net_maze.c
// Run:   ./net_maze
// Connect: nc <server-ip> 1234

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define PORT 1234
#define MAZE_W 21
#define MAZE_H 11

static const char *maze_template[MAZE_H] = {
    "#####################",
    "#   #       #       #",
    "# # # ##### # ##### #",
    "# #   #   #   #   # #",
    "# ##### # ##### # # #",
    "#     # #     # #   #",
    "##### # ##### # #####",
    "#   #   #   # #     #",
    "# # ##### # # ##### #",
    "#     #   #   #   E #",
    "#####################"
};

static void send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) { if (n < 0 && errno == EINTR) continue; break; }
        sent += (size_t)n;
    }
}

int main(void) {
    int server_fd = -1, client_fd = -1;
    struct sockaddr_in addr, caddr;
    socklen_t clen = sizeof(caddr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); close(server_fd); return 1; }
    if (listen(server_fd, 4) < 0) { perror("listen"); close(server_fd); return 1; }

    printf("Listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&caddr, &clen);
        if (client_fd < 0) { perror("accept"); continue; }

        const char *hdr = "Maze (view only)\n\n";
        send_all(client_fd, hdr, strlen(hdr));
        for (int y = 0; y < MAZE_H; ++y) {
            send_all(client_fd, maze_template[y], strlen(maze_template[y]));
            send_all(client_fd, "\n", 1);
        }
        send_all(client_fd, "\n", 1);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

