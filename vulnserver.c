// Version 2/30
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

int main(void){
    printf("Listening (not implemented) on %d\n", PORT);
    return 0;
}

