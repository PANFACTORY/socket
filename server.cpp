//  Server side
#include <iostream>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <errno.h>

int main() {
    //  (0) winsockの初期化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) == SOCKET_ERROR) {
        std::cout << "Error initialising WSA." << std::endl;
        return -1;
    }

    //  (1) ソケットの作成
    int sock0 = socket(AF_INET, SOCK_STREAM, 0);    //  socket(アドレスの種類, ソケットのタイプ, プロトコル)
    if (sock0 < 0) {
        perror("socket");
        std::cout << errno << std::endl;
        return 1;
    }
    std::cout << "after scoket" << std::endl;

    //  (2) ソケットのバインド
    struct sockaddr_in addr;                        //  IPv4のソケットアドレスタイプの構造体
    addr.sin_family = AF_INET;                      //  アドレスの種類
    addr.sin_port = htons(50000);                   //  ポート番号
    addr.sin_addr.s_addr = INADDR_ANY;              //  アドレス
    if (bind(sock0, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        perror("bind");
        return 1;
    }

    //  (3) ソケットを接続待ちの状態にする
    int backlog = 5;                                //  保留中のキューの最大値
    if (listen(sock0, backlog) != 0) {
        perror("listen");
        return 1;
    }

    //  (4) クライアントからの接続を受ける
    while (1) {
        //  (4-A)   クライアントから通信接続要求が来るまでプログラムを停止
        struct sockaddr_in client;
        int len = sizeof(client);
        int sock = accept(sock0, (struct sockaddr*)&client, &len);
        if (sock < 0) {
            perror("accept");
            break;
        }

        //  (4-B)   クライアントにメッセージを送信
        if (send(sock, "RDCS SERVER", 11, 0) < 1) {
            perror("write");
            break;
        }

        //  (4-C)   ソケットを閉じて通信接続を終了する
        closesocket(sock);
    }
 
    //  (5) サーバーのソケットを閉じる
    closesocket(sock0);

    //  (6) winsockの終了
    WSACleanup();
    
    return 0;
}