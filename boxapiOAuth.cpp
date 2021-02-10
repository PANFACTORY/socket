//  Link option -lwsock32 -lws2_32 -lssl -lcrypto
#include <iostream>
#include <string>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int main() {
    std::string hostname = "account.box.com";
    std::string hostname2 = "api.box.com/oauth2/token";
    std::string service = "https";
    std::string clientid;

    std::cout << "client id:";
    std::cin >> clientid;

    //  (1) winsockの初期化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) == SOCKET_ERROR) {
        std::cout << "Error initialising WSA." << std::endl;
        return -1;
    }   //  winsockの初期化を行わないとエラー10093が出る

    //*************************************************************************

    //  (2) urlから接続情報を取得
    struct addrinfo hints, *res;
    struct in_addr addr;
    int err;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;  //  IPv4:AF_INET, IPv6:AF_INET6, IPv4&IPv6:PF_UNSPEC
    if ((err = getaddrinfo(hostname.c_str(), service.c_str(), &hints, &res)) != 0) {
        std::cout << "error " << err << std::endl;
        return 1;
    }
    addr.s_addr= ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    //std::cout << "ip addres: " << inet_ntoa(addr) << std::endl;
    
    //  (3) ソケットの作成
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    //  (4) サーバーに接続を要求
    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        perror("connect");
        return 1;
    }

    //  (5) SSLに関する設定
    SSL_load_error_strings();
    SSL_library_init();

    SSL_CTX *ctx;
    ctx = SSL_CTX_new(SSLv23_client_method());
    SSL *ssl;
    ssl = SSL_new(ctx);
    err = SSL_set_fd(ssl, sock);
    SSL_connect(ssl);

    //  (6) サーバーにメッセージを送信
    char send_buf[256];
    sprintf(send_buf, "GET /api/oauth2/authorize?client_id=%s&response_type=code HTTP/1.0\r\n", clientid.c_str());
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf, "Host: account.box.com\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf, "content-type: application/json\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf, "\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf, "\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));

    //  (7) サーバーからメッセージを受け取る
    std::string recstr;
    while (1){
        char rec_buf[256];
        int read_size;
        read_size = SSL_read(ssl, rec_buf, 256);
        if (read_size > 0){
            for (int i = 0; i < read_size; i++) {
                recstr += rec_buf[i];
            }
        } else {
            break;
        }
    }

    //  (8) 切断
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    ERR_free_strings();
    closesocket(sock);
    freeaddrinfo(res);  // 取得した情報を解放

    //*************************************************************************
    
    //  (1) ソケットの作成
    int sock0 = socket(AF_INET, SOCK_STREAM, 0);    //  socket(アドレスの種類, ソケットのタイプ, プロトコル) -> ファイルディスクリプタ
    if (sock0 < 0) {
        perror("socket");
        std::cout << errno << std::endl;
        return 1;
    }

    //  (2) ソケットのバインド
    int portnumber = 5000;
    struct sockaddr_in addr1;                        //  IPv4のソケットアドレスタイプの構造体
    addr1.sin_family = AF_INET;                      //  アドレスの種類
    addr1.sin_port = htons(portnumber);              //  ポート番の設定，htons()でネットワークバイトオーダーを解決
    addr1.sin_addr.s_addr = INADDR_ANY;              //  アドレス
    
    if (bind(sock0, (struct sockaddr*)&addr1, sizeof(addr1)) != 0) {
        perror("bind");
        return 1;
    }
    
    //  (3) ソケットを接続待ちの状態にする
    int backlog = 5;                                //  保留中のキューの最大値
    if (listen(sock0, backlog) != 0) {
        perror("listen");
        return 1;
    }    
    std::cout << "Access to: http://localhost:" << portnumber << std::endl;
    
    
    //  (4) クライアントからの接続を受ける
    struct sockaddr_in client;
    int len = sizeof(client);
    int sock3 = accept(sock0, (struct sockaddr*)&client, &len);
    if (sock3 < 0) {
        perror("accept");
    } else {
        char buf[1024] = { 0 };
        int get = recv(sock3, buf, 1024, 0);
        //std::cout << buf << std::endl;

        if (send(sock3, recstr.c_str(), recstr.size(), 0) < 1) {
            perror("write");
        }
        closesocket(sock3);
    }

    //  (5) サーバーのソケットを閉じる
    closesocket(sock0);

    //*************************************************************************
    //  (1) ソケットの作成
    int sock1 = socket(AF_INET, SOCK_STREAM, 0);    //  socket(アドレスの種類, ソケットのタイプ, プロトコル) -> ファイルディスクリプタ
    if (sock1 < 0) {
        perror("socket");
        std::cout << errno << std::endl;
        return 1;
    }

    //  (2) ソケットのバインド
    struct sockaddr_in addr2;                        //  IPv4のソケットアドレスタイプの構造体
    addr2.sin_family = AF_INET;                      //  アドレスの種類
    addr2.sin_port = htons(portnumber);              //  ポート番の設定，htons()でネットワークバイトオーダーを解決
    addr2.sin_addr.s_addr = INADDR_ANY;              //  アドレス
    
    if (bind(sock1, (struct sockaddr*)&addr2, sizeof(addr2)) != 0) {
        perror("bind");
        return 1;
    }
    
    //  (3) ソケットを接続待ちの状態にする
    if (listen(sock1, backlog) != 0) {
        perror("listen");
        return 1;
    }    
    
    //  (4) クライアントからの接続を受ける
    while(1) {
        struct sockaddr_in client2;
        int len2 = sizeof(client2);
        int sock4 = accept(sock0, (struct sockaddr*)&client2, &len2);
        if (sock4 < 0) {
            perror("accept");
        } else {
            char buf[1024] = { 0 };
            int get = recv(sock4, buf, 1024, 0);
            std::cout << buf << std::endl;
            closesocket(sock4);
        }
    }
    
    closesocket(sock1);

/*
    //  (2) urlから接続情報を取得
    struct addrinfo hints2, *res2;
    struct in_addr addr2;
    int err2;
    memset(&hints2, 0, sizeof(hints2));
    hints2.ai_socktype = SOCK_STREAM;
    hints2.ai_family = AF_INET;  //  IPv4:AF_INET, IPv6:AF_INET6, IPv4&IPv6:PF_UNSPEC
    if ((err2 = getaddrinfo(hostname.c_str(), service.c_str(), &hints2, &res2)) != 0) {
        std::cout << "error " << err2 << std::endl;
        return 1;
    }
    addr2.s_addr= ((struct sockaddr_in *)(res2->ai_addr))->sin_addr.s_addr;
    //std::cout << "ip addres: " << inet_ntoa(addr) << std::endl;
    
    //  (3) ソケットの作成
    int sock2 = socket(res2->ai_family, res2->ai_socktype, res2->ai_protocol);
    if (sock2 < 0) {
        perror("socket");
        return 1;
    }

    //  (4) サーバーに接続を要求
    if (connect(sock2, res2->ai_addr, res2->ai_addrlen) != 0) {
        perror("connect");
        return 1;
    }

    //  (5) SSLに関する設定
    SSL_load_error_strings();
    SSL_library_init();

    SSL_CTX *ctx2;
    ctx2 = SSL_CTX_new(SSLv23_client_method());
    SSL *ssl2;
    ssl2 = SSL_new(ctx2);
    err2 = SSL_set_fd(ssl2, sock2);
    SSL_connect(ssl2);

    //  (6) サーバーにメッセージを送信
    char send_buf2[256];
    sprintf(send_buf2, "POST /oauth2/token HTTP/1.0\r\n");
    SSL_write(ssl2, send_buf2, strlen(send_buf2));
    sprintf(send_buf2, "Host: api.box.com\r\n");
    SSL_write(ssl2, send_buf2, strlen(send_buf2));
    sprintf(send_buf2, "content-type: application/json\r\n");
    SSL_write(ssl2, send_buf2, strlen(send_buf2));
    sprintf(send_buf2, "\r\n");
    SSL_write(ssl2, send_buf2, strlen(send_buf2));
    sprintf(send_buf, "client_id=%s&client_secret=%s&grant_type=authorization_code&code=%s\r\n", clientid.c_str(), );
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf2, "\r\n");
    SSL_write(ssl2, send_buf2, strlen(send_buf2));

    //  (7) サーバーからメッセージを受け取る
    std::string recstr2;
    while (1){
        char rec_buf2[256];
        int read_size2;
        read_size2 = SSL_read(ssl2, rec_buf2, 256);
        if (read_size2 > 0){
            for (int i = 0; i < read_size2; i++) {
                recstr2 += rec_buf2[i];
            }
        } else {
            break;
        }
    }

    //  (8) 切断
    SSL_shutdown(ssl2);
    SSL_free(ssl2);
    SSL_CTX_free(ctx2);
    ERR_free_strings();
    closesocket(sock2);
    freeaddrinfo(res2);  // 取得した情報を解放
*/    
    WSACleanup();

    return 0;
}