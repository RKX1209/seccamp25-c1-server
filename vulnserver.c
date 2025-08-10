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
void draw_and_send_maze(int fd, char maze[MAZE_H][MAZE_W+1], const char *name, int px, int py) {
    char out[MAX_LEN]; int len=0; len+=snprintf(out+len,sizeof(out)-len,"\nPlayer: %s\n",name);
    for(int y=0;y<MAZE_H && len<(int)sizeof(out)-1;++y){ for(int x=0;x<MAZE_W && len<(int)sizeof(out)-1;++x){ out[len++]=(x==px&&y==py)?'@':maze[y][x]; } out[len++]='\n'; }
    out[len++]='\n'; out[len]=0; send_all(fd,out,(size_t)len);
}

int main(void){
    int sfd=socket(AF_INET,SOCK_STREAM,0); if(sfd<0){perror("socket"); return 1;}
    int opt=1; setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    struct sockaddr_in a; memset(&a,0,sizeof(a)); a.sin_family=AF_INET; a.sin_port=htons(PORT); a.sin_addr.s_addr=INADDR_ANY;
    if(bind(sfd,(struct sockaddr*)&a,sizeof(a))<0){perror("bind"); close(sfd); return 1;}
    if(listen(sfd,4)<0){perror("listen"); close(sfd); return 1;}
    printf("Listening on port %d...\n", PORT);

    while(1){
        struct sockaddr_in ca; socklen_t clen=sizeof(ca);
        int cfd=accept(sfd,(struct sockaddr*)&ca,&clen); if(cfd<0){perror("accept"); continue;}

        const char*welcome="Welcome to maze server! Send your player name (max 31 chars):\n";
        send_all(cfd,welcome,strlen(welcome));
        char line[MAX_LEN]; ssize_t n=recv_line(cfd,line,sizeof(line)); if(n<=0){close(cfd); continue;}

        char name[NAME_MAX]; strncpy(name,line,NAME_MAX-1); name[NAME_MAX-1]=0;

        char maze[MAZE_H][MAZE_W+1]; for(int y=0;y<MAZE_H;++y){strncpy(maze[y],maze_template[y],MAZE_W); maze[y][MAZE_W]=0;}
        int px=1, py=1;

        const char*instr="Controls: w=up, s=down, a=left, d=right, q=quit\nReach 'E' to win.\n";
        send_all(cfd,instr,strlen(instr));
        draw_and_send_maze(cfd,maze,name,px,py);

        while(1){
            const char*prompt="Your move> "; send_all(cfd,prompt,strlen(prompt));
            char buf[64]; ssize_t r=recv(cfd,buf,sizeof(buf)-1,0); if(r<=0) break; buf[r]=0;
            char cmd=0; for(ssize_t i=0;i<r;++i){ if(buf[i]=='\r'||buf[i]=='\n'||buf[i]==' '||buf[i]=='\t') continue; cmd=buf[i]; break; }
            if(!cmd) continue;
            int nx=px, ny=py;
            if(cmd=='q'||cmd=='Q'){ const char*bye="Goodbye!\n"; send_all(cfd,bye,strlen(bye)); break; }
            else if(cmd=='w'||cmd=='W') ny=py-1;
            else if(cmd=='s'||cmd=='S') ny=py+1;
            else if(cmd=='a'||cmd=='A') nx=px-1;
            else if(cmd=='d'||cmd=='D') nx=px+1;
            else { const char*unk="Unknown command. Use w/a/s/d to move, q to quit.\n"; send_all(cfd,unk,strlen(unk)); continue; }

            if(nx<0||nx>=MAZE_W||ny<0||ny>=MAZE_H){ const char*oob="Can't move outside maze.\n"; send_all(cfd,oob,strlen(oob)); continue; }
            if(maze[ny][nx]=='#'){ const char*wall="Hit a wall.\n"; send_all(cfd,wall,strlen(wall)); continue; }

            px=nx; py=ny;

            if(maze[py][px]=='E'){
                char win[128]; int wn=snprintf(win,sizeof(win),"\nCongratulations %s! You reached the exit!\n\n",name);
                send_all(cfd,win,(size_t)wn);
                draw_and_send_maze(cfd,maze,name,px,py);
                break;
            }

            draw_and_send_maze(cfd,maze,name,px,py);
        }
        close(cfd);
    }
    close(sfd); return 0;
}

