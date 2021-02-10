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
    std::string clientid, clientsecret, code;
    int portnumber = 5000, err, backlog = 5;                                //  保留中のキューの最大値

    std::cout << "client id:";
    std::cin >> clientid;
    std::cout << "client secret:";
    std::cin >> clientsecret;

    //  winsockの初期化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) == SOCKET_ERROR) {
        std::cout << "Error initialising WSA." << std::endl;
        return -1;
    }   //  winsockの初期化を行わないとエラー10093が出る

    //********************STEP1（承認URLを構成する）********************
    struct addrinfo hints0, *res0;
    struct in_addr addr0;
    memset(&hints0, 0, sizeof(hints0));
    hints0.ai_socktype = SOCK_STREAM;
    hints0.ai_family = AF_INET;  //  IPv4:AF_INET, IPv6:AF_INET6, IPv4&IPv6:PF_UNSPEC
    if ((err = getaddrinfo(hostname.c_str(), service.c_str(), &hints0, &res0)) != 0) {
        std::cout << "error " << err << std::endl;
        return 1;
    }
    addr0.s_addr= ((struct sockaddr_in *)(res0->ai_addr))->sin_addr.s_addr;
    
    int sock0 = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol);
    if (sock0 < 0) {
        perror("socket");
        return 1;
    }

    if (connect(sock0, res0->ai_addr, res0->ai_addrlen) != 0) {
        perror("connect");
        return 1;
    }

    SSL_load_error_strings();
    SSL_library_init();
    SSL_CTX *ctx0;
    ctx0 = SSL_CTX_new(SSLv23_client_method());
    SSL *ssl0;
    ssl0 = SSL_new(ctx0);
    err = SSL_set_fd(ssl0, sock0);
    SSL_connect(ssl0);

    char send_buf0[256];
    sprintf(send_buf0, "GET /api/oauth2/authorize?client_id=%s&response_type=code HTTP/1.0\r\n", clientid.c_str());
    SSL_write(ssl0, send_buf0, strlen(send_buf0));
    sprintf(send_buf0, "Host: account.box.com\r\n");
    SSL_write(ssl0, send_buf0, strlen(send_buf0));
    sprintf(send_buf0, "content-type: application/json\r\n");
    SSL_write(ssl0, send_buf0, strlen(send_buf0));
    sprintf(send_buf0, "\r\n");
    SSL_write(ssl0, send_buf0, strlen(send_buf0));
    sprintf(send_buf0, "\r\n");
    SSL_write(ssl0, send_buf0, strlen(send_buf0));

    std::string redirectcontent;
    while (1){
        char rec_buf[256];
        int read_size;
        read_size = SSL_read(ssl0, rec_buf, 256);
        if (read_size > 0){
            for (int i = 0; i < read_size; i++) {
                redirectcontent += rec_buf[i];
            }
        } else {
            break;
        }
    }

    SSL_shutdown(ssl0);
    SSL_free(ssl0);
    SSL_CTX_free(ctx0);
    ERR_free_strings();
    closesocket(sock0);
    freeaddrinfo(res0);

    //********************STEP2（ユーザーをリダイレクトする）********************
    int sock1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock1 < 0) {
        perror("socket");
        std::cout << errno << std::endl;
        return 1;
    }

    struct sockaddr_in addr1;                        //  IPv4のソケットアドレスタイプの構造体
    addr1.sin_family = AF_INET;                      //  アドレスの種類
    addr1.sin_port = htons(portnumber);              //  ポート番の設定，htons()でネットワークバイトオーダーを解決
    addr1.sin_addr.s_addr = INADDR_ANY;              //  アドレス
    
    if (bind(sock1, (struct sockaddr*)&addr1, sizeof(addr1)) != 0) {
        perror("bind");
        return 1;
    }
        
    if (listen(sock1, backlog) != 0) {
        perror("listen");
        return 1;
    }    
    std::cout << "Access to: http://localhost:" << portnumber << std::endl;
    
    while(1) {
        struct sockaddr_in client1;
        int len1 = sizeof(client1);
        int tmpsock1 = accept(sock1, (struct sockaddr*)&client1, &len1);
        if (tmpsock1 < 0) {
            perror("accept");
            break;
        } else {
            char buf[4096] = { 0 };
            int get = recv(tmpsock1, buf, 4096, 0);
            char *iscode;
            iscode = strstr(buf, "?code=");

            //  ここでリクエストを解析してGETパラメータにcodeが含まれている場合はループを抜ける
            if (iscode) {
                iscode += 6;
                while (*iscode != ' ' && *iscode != '\0') {
                    code += *iscode;
                    iscode++;                    
                }
                std::string msg("HTTP/1.0 200 OK\r\ntext/html\r\n\r\nHello world");
                if (send(tmpsock1, msg.c_str(), msg.size(), 0) < 1) {
                    perror("write");
                    return 1;
                }
                closesocket(tmpsock1);
                break;
            } else {
                if (send(tmpsock1, redirectcontent.c_str(), redirectcontent.size(), 0) < 1) {
                    perror("write");
                    return 1;
                }
                closesocket(tmpsock1);
            }
        }
    }

    closesocket(sock1);

    //********************STEP4（コードを交換する）********************
    struct addrinfo hints2, *res2;
    struct in_addr addr2;
    memset(&hints2, 0, sizeof(hints2));
    hints2.ai_socktype = SOCK_STREAM;
    hints2.ai_family = AF_INET;  //  IPv4:AF_INET, IPv6:AF_INET6, IPv4&IPv6:PF_UNSPEC
    if ((err = getaddrinfo(hostname.c_str(), service.c_str(), &hints2, &res2)) != 0) {
        std::cout << "error " << err << std::endl;
        return 1;
    }
    addr2.s_addr= ((struct sockaddr_in *)(res2->ai_addr))->sin_addr.s_addr;
    
    int sock2 = socket(res2->ai_family, res2->ai_socktype, res2->ai_protocol);
    if (sock2 < 0) {
        perror("socket");
        return 1;
    }

    if (connect(sock2, res2->ai_addr, res2->ai_addrlen) != 0) {
        perror("connect");
        return 1;
    }

    SSL_load_error_strings();
    SSL_library_init();
    SSL_CTX *ctx2;
    ctx2 = SSL_CTX_new(SSLv23_client_method());
    SSL *ssl2;
    ssl2 = SSL_new(ctx2);
    err = SSL_set_fd(ssl2, sock2);
    SSL_connect(ssl2);

    char send_buf2[256];
    sprintf(send_buf2, "POST /oauth2/token HTTP/1.0\r\n");
    SSL_write(ssl2, send_buf2, strlen(send_buf2));
    sprintf(send_buf2, "Host: api.box.com\r\n");
    SSL_write(ssl2, send_buf2, strlen(send_buf2));
    sprintf(send_buf2, "content-type: application/x-www-form-urlencoded\r\n");
    SSL_write(ssl2, send_buf2, strlen(send_buf2));
    sprintf(send_buf2, "\r\n");
    SSL_write(ssl2, send_buf2, strlen(send_buf2));
    std::string strencoded = "grant_type%3Dauthorization_code%26code%3D" + code + "%26client_id%3D" + clientid + "%26client_secret%3D" + clientsecret;
    sprintf(send_buf2, "%s\r\n", strencoded.c_str());
    SSL_write(ssl2, send_buf2, strlen(send_buf2));
    sprintf(send_buf2, "\r\n");
    SSL_write(ssl2, send_buf2, strlen(send_buf2));
    
    std::string retcontent;
    while (1){
        char rec_buf[256];
        int read_size;
        read_size = SSL_read(ssl2, rec_buf, 256);
        if (read_size > 0){
            for (int i = 0; i < read_size; i++) {
                retcontent += rec_buf[i];
            }
        } else {
            break;
        }
    }

    SSL_shutdown(ssl2);
    SSL_free(ssl2);
    SSL_CTX_free(ctx2);
    ERR_free_strings();
    closesocket(sock2);
    freeaddrinfo(res2);

    std::cout << retcontent << std::endl;

    //  winsockの終了
    WSACleanup(); 
}