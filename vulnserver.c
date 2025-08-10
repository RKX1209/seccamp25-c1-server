#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define MAX_LEN 512
#define PORT 4444
#define NAME_MAX 32
#define MAZE_W 21
#define MAZE_H 11

// Simple maze: '#' wall, ' ' floor, 'E' exit
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
        if (n <= 0) {
            // if interrupted by signal, try again
            if (n < 0 && errno == EINTR) continue;
            break;
        }
        sent += (size_t)n;
    }
}

ssize_t recv_line(int fd, char *buf, size_t maxlen) {
    // Read until newline or maxlen-1 reached. Returns bytes read (excluding null).
    size_t idx = 0;
    while (idx < maxlen - 1) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n == 1) {
            if (c == '\r') continue; // skip CR
            if (c == '\n') break;
            buf[idx++] = c;
        } else if (n == 0) {
            // connection closed
            break;
        } else {
            if (errno == EINTR) continue;
            return -1;
        }
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
            if (x == px && y == py) {
                out[len++] = '@'; // player character
            } else {
                out[len++] = maze[y][x];
            }
        }
        out[len++] = '\n';
    }
    out[len++] = '\n';
    out[len] = '\0';
    send_all(client_fd, out, (size_t)len);
}

int main(void) {
    int server_fd = -1, client_fd = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char linebuf[MAX_LEN];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 4) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Accepted connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        // Send welcome and ask for name
        const char *welcome_msg = "Welcome to maze server! Send your player name (max 31 chars):\n";
        send_all(client_fd, welcome_msg, strlen(welcome_msg));

        // Read name line safely
        ssize_t n = recv_line(client_fd, linebuf, sizeof(linebuf));
        if (n <= 0) {
            printf("Client disconnected before sending name.\n");
            close(client_fd);
            continue;
        }

        // sanitize name: trim whitespace, limit length
        char player_name[NAME_MAX];
        // remove leading spaces
        char *p = linebuf;
        while (*p == ' ' || *p == '\t') ++p;
        // copy up to NAME_MAX-1
        strncpy(player_name, p, NAME_MAX-1);
        player_name[NAME_MAX-1] = '\0';
        // remove trailing spaces/newlines
        for (int i = (int)strlen(player_name) - 1; i >= 0; --i) {
            if (player_name[i] == ' ' || player_name[i] == '\t' || player_name[i] == '\r' || player_name[i] == '\n') player_name[i] = '\0';
            else break;
        }
        if (player_name[0] == '\0') {
            strncpy(player_name, "player", NAME_MAX-1);
            player_name[NAME_MAX-1] = '\0';
        }

        // Build working maze (copy template to mutable array)
        char maze[MAZE_H][MAZE_W+1];
        for (int y = 0; y < MAZE_H; ++y) {
            strncpy(maze[y], maze_template[y], MAZE_W);
            maze[y][MAZE_W] = '\0';
        }

        // Find starting position (we'll use a fixed start at (1,1) if possible)
        int px = 1, py = 1;
        if (maze[py][px] == '#') {
            // fallback: find any space
            int found = 0;
            for (int y = 0; y < MAZE_H && !found; ++y) {
                for (int x = 0; x < MAZE_W; ++x) {
                    if (maze[y][x] == ' ') {
                        py = y; px = x; found = 1; break;
                    }
                }
            }
        }

        // Send initial instructions
        const char *instr = "Controls: w=up, s=down, a=left, d=right, q=quit\nReach 'E' to win.\n";
        send_all(client_fd, instr, strlen(instr));

        // Draw initial maze
        draw_and_send_maze(client_fd, maze, player_name, px, py);

        // Game loop
        int game_over = 0;
        while (!game_over) {
            // Prompt
            const char *prompt = "Your move> ";
            send_all(client_fd, prompt, strlen(prompt));

            ssize_t r = recv(client_fd, linebuf, sizeof(linebuf)-1, 0);
            if (r <= 0) {
                if (r == 0) {
                    printf("Client %s disconnected.\n", client_ip);
                } else {
                    perror("recv");
                }
                break;
            }
            // Treat first non-whitespace char as command
            linebuf[r] = '\0';
            char cmd = '\0';
            for (ssize_t i = 0; i < r; ++i) {
                if (linebuf[i] == '\r' || linebuf[i] == '\n') continue;
                if (linebuf[i] == ' ' || linebuf[i] == '\t') continue;
                cmd = linebuf[i];
                break;
            }
            if (cmd == '\0') continue;

            int nx = px, ny = py;
            if (cmd == 'w' || cmd == 'W') ny = py - 1;
            else if (cmd == 's' || cmd == 'S') ny = py + 1;
            else if (cmd == 'a' || cmd == 'A') nx = px - 1;
            else if (cmd == 'd' || cmd == 'D') nx = px + 1;
            else if (cmd == 'q' || cmd == 'Q') {
                const char *bye = "Goodbye!\n";
                send_all(client_fd, bye, strlen(bye));
                break;
            } else {
                const char *unk = "Unknown command. Use w/a/s/d to move, q to quit.\n";
                send_all(client_fd, unk, strlen(unk));
                continue;
            }

            // Check bounds and wall
            if (nx < 0 || nx >= MAZE_W || ny < 0 || ny >= MAZE_H) {
                const char *oob = "Can't move outside maze.\n";
                send_all(client_fd, oob, strlen(oob));
                continue;
            }
            if (maze[ny][nx] == '#') {
                const char *wall = "Hit a wall.\n";
                send_all(client_fd, wall, strlen(wall));
                continue;
            }

            // Move
            px = nx; py = ny;

            // Check win
            if (maze[py][px] == 'E') {
                char winmsg[128];
                int wn = snprintf(winmsg, sizeof(winmsg), "\nCongratulations %s! You reached the exit!\n\n", player_name);
                send_all(client_fd, winmsg, (size_t)wn);
                draw_and_send_maze(client_fd, maze, player_name, px, py);
                game_over = 1;
                break;
            }

            // Send updated maze
            draw_and_send_maze(client_fd, maze, player_name, px, py);
        }

        close(client_fd);
        client_fd = -1;
        printf("Connection closed for %s\n", client_ip);
    }

    close(server_fd);
    return 0;
}
