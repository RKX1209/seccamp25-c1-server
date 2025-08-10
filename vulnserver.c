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

void draw_and_send_maze(int client_fd, char maze[MAZE_H][MAZE_W+1], const char *name, int px, int py) {
    char out[MAX_LEN];
    int len = 0;
    len += snprintf(out + len, sizeof(out) - len, "\nPlayer: %s\n", name);
    for (int y = 0; y < MAZE_H && len < (int)sizeof(out) - 1; ++y) {
        for (int x = 0; x < MAZE_W && len < (int)sizeof(out) - 1; ++x) {
            if (x == px && y == py) out[len++] = '@';
            else out[len++] = maze[y][x];
        }
        out[len++] = '\n';
    }
    out[len++] = '\n';
    out[len] = '\0';
    send_all(client_fd, out, (size_t)len);
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
        if (n <= 0) { close(cfd); continue; }

        char player_name[NAME_MAX];
        strncpy(player_name, linebuf, NAME_MAX-1); player_name[NAME_MAX-1] = 0;

        char maze[MAZE_H][MAZE_W+1];
        for (int y = 0; y < MAZE_H; ++y) { strncpy(maze[y], maze_template[y], MAZE_W); maze[y][MAZE_W] = '\0'; }

        int px = 1, py = 1;
        draw_and_send_maze(cfd, maze, player_name, px, py);

        close(cfd);
    }
    close(server_fd);
    return 0;
}

