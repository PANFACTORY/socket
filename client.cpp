//  Client side
#include <iostream>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>

int main(int argc, char *argv[]) {
    //  (0-A) アドレス取得，コマンドライン引数に渡すこと 
    if (argc != 2) {
        std::cout << "Usage : " << argv[0] << " dest" << std::endl;
        return 1;
    }
    std::string deststr(argv[1]);
 
    //  (0-B) winsockの初期化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) == SOCKET_ERROR) {
        std::cout << "Error initialising WSA." << std::endl;
        return -1;
    }

    //  (1) ソケットの作成
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    //  (2) サーバーに接続を要求
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(50000);
    server.sin_addr.s_addr = inet_addr(deststr.c_str());
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
        perror("connect");
        return 1;
    }

    //  (3) サーバーからメッセージを受け取る
    char vect[512] = { 0 };
    int get = recv(sock, vect, 512, 0);
    std::cout << "read=" << vect << std::endl;
 
    //  (4) 切断
    closesocket(sock);

    //  (5) winsockの終了
    WSACleanup();

    return 0;
}