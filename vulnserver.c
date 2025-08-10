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

static void send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) { if (n < 0 && errno == EINTR) continue; break; }
        sent += (size_t)n;
    }
}

static ssize_t recv_line(int fd, char *buf, size_t maxlen) {
    size_t idx = 0;
    while (idx < maxlen - 1) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n == 1) {
            if (c == '\r') continue;
            if (c == '\n') break;
            buf[idx++] = c;
        } else if (n == 0) {
            break;
        } else {
            if (errno == EINTR) continue;
            return -1;
        }
    }
    buf[idx] = '\0';
    return (ssize_t)idx;
}

static void draw_maze_named(int fd, int px, int py, const char *name) {
    char hdr[128];
    int n = snprintf(hdr, sizeof(hdr), "\nPlayer: %s\n", name ? name : "player");
    send_all(fd, hdr, (size_t)n);

    char line[64];
    for (int y = 0; y < MAZE_H; ++y) {
        for (int x = 0; x < MAZE_W; ++x) {
            char c = maze_template[y][x];
            if (x == px && y == py) c = '@';
            line[x] = c;
        }
        line[MAZE_W] = '\0';
        send_all(fd, line, strlen(line));
        send_all(fd, "\n", 1);
    }
    send_all(fd, "\n", 1);
}

int main(void) {
    int server_fd = -1, client_fd = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char linebuf[MAX_LEN];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) { perror("bind"); close(server_fd); return 1; }
    if (listen(server_fd, 4) < 0) { perror("listen"); close(server_fd); return 1; }

    printf("Listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) { perror("accept"); continue; }

        const char *welcome = "Enter your name:\n";
        send_all(client_fd, welcome, strlen(welcome));
        ssize_t n = recv_line(client_fd, linebuf, sizeof(linebuf));
        if (n <= 0) { close(client_fd); continue; }

        char name[NAME_MAX];
        strncpy(name, linebuf, NAME_MAX-1);
        name[NAME_MAX-1] = '\0';

        const char *inst = "Controls: w/a/s/d, q to quit\n";
        send_all(client_fd, inst, strlen(inst));

        int px = 1, py = 1;
        draw_maze_named(client_fd, px, py, name);

        while (1) {
            send_all(client_fd, "Your move> ", 11);
            ssize_t r = recv_line(client_fd, linebuf, sizeof(linebuf));
            if (r <= 0) break;
            char c = linebuf[0];

            if (c == 'q' || c == 'Q') break;

            int nx = px, ny = py;
            if (c == 'w' || c == 'W') --ny;
            else if (c == 's' || c == 'S') ++ny;
            else if (c == 'a' || c == 'A') --nx;
            else if (c == 'd' || c == 'D') ++nx;

            if (nx >= 0 && nx < MAZE_W && ny >= 0 && ny < MAZE_H && maze_template[ny][nx] != '#') {
                px = nx; py = ny;
            }
            draw_maze_named(client_fd, px, py, name);
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}

