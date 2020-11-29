//  Link option -lwsock32 -lws2_32 -lssl -lcrypto
#include <iostream>
#include <string>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int main() {
    std::string hostname = "api.box.com";
    std::string service = "https";
    
    //  (1) winsockの初期化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) == SOCKET_ERROR) {
        std::cout << "Error initialising WSA." << std::endl;
        return -1;
    }   //  winsockの初期化を行わないとエラー10093が出る

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
    sprintf(send_buf, "POST /oauth2/token HTTP/1.0\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf, "Host: api.box.com\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf, "content-type: application/x-www-form-urlencoded\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf, "content-length: <content_length>\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf, "\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf, "client_id=<client_id>&client_secret=<client_secret>&grant_type=client_credentials&box_subject_type=enterprise&box_subject_id=<enterprise_id>\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));
    sprintf(send_buf, "\r\n");
    SSL_write(ssl, send_buf, strlen(send_buf));

    //  (7) サーバーからメッセージを受け取る
    while (1){
        char rec_buf[256];
        int read_size;
        read_size = SSL_read(ssl, rec_buf, 256);
        if (read_size > 0){
            for (int i = 0; i < read_size; i++) {
                std::cout << rec_buf[i];
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
    WSACleanup();

    return 0;
}