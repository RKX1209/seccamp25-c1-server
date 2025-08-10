#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_LEN 512
#define PORT 4444

void process_input(const char *input) {
    char buf[128];
    memcpy(buf, input, MAX_LEN);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char input[MAX_LEN];

    // ソケット作成
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // アドレス情報設定
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // バインド
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 待ち受け開始
    if (listen(server_fd, 1) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

	// 🎉 Welcome メッセージ送信
        const char *welcome_msg = "Welcome to seccmp25 C1 server! Send your input:\n";
        write(client_fd, welcome_msg, strlen(welcome_msg));

        while (1) {
            memset(input, 0, MAX_LEN);
            ssize_t received = recv(client_fd, input, MAX_LEN - 1, 0);
            if (received <= 0) {
                printf("Client disconnected.\n");
                break;
            }

            // 標準出力にログ
            printf("[*] Received %zd bytes\n", received);

            // 入力処理（脆弱性のある関数）
            process_input(input);
        }
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
