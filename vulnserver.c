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

void send_all(int fd, const char *buf, size_t len) { size_t s=0; while(s<len){ssize_t n=send(fd,buf+s,len-s,0); if(n<=0)break; s+=(size_t)n;} }
ssize_t recv_line(int fd, char *buf, size_t maxlen) { size_t i=0; while(i<maxlen-1){ char c; ssize_t n=recv(fd,&c,1,0); if(n==1){ if(c=='\r')continue; if(c=='\n')break; buf[i++]=c; } else if(n==0)break; else return -1; } buf[i]=0; return (ssize_t)i; }
void draw_and_send_maze(int fd, char maze[

