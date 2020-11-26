#include <iostream>
#include <string>

#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>

int main(int argc, char **args) {
    std::string hostname = "www.google.com";
    std::string service = "http";
    struct addrinfo hints, *res;
    struct in_addr addr;
    int err;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) == SOCKET_ERROR) {
        std::cout << "Error initialising WSA." << std::endl;
        return -1;
    }   //  winsockの初期化を行わないとエラー10093が出る

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;  //  IPv4:AF_INET, IPv6:AF_INET6, IPv4&IPv6:PF_UNSPEC

    if ((err = getaddrinfo(hostname.c_str(), service.c_str(), &hints, &res)) != 0) {
        std::cout << "error " << err << std::endl;
        return 1;
    }

    addr.s_addr= ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    std::cout << "ip addres: " << inet_ntoa(addr) << std::endl;

    //  (1) ソケットの作成
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    //  (2) サーバーに接続を要求
    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        perror("connect");
        return 1;
    }

    //  (3-A) サーバーにメッセージを送信
    std::string msg = "GET / HTTP/1.1\r\nHost: " + hostname + "\r\nConnection: close\r\n\r\n";
    if (send(sock, msg.c_str(), msg.size(), 0) < 1) {
        perror("write");
        return 1;
    }

    //  (3-B) サーバーからメッセージを受け取る
    char buffer[10000];
    int nDataLength;
    std::string website_HTML;
    while ((nDataLength = recv(sock, buffer, 10000, 0)) > 0) {
        int i = 0;
        while (buffer[i] >= 32 || buffer[i] == '\n' || buffer[i] == '\r') {
            website_HTML += buffer[i];
            i += 1;
        }              
    }
    // Display HTML source
    std::cout << website_HTML;

    //  (4) 切断
    closesocket(sock);

    // 取得した情報を解放
    freeaddrinfo(res);

    WSACleanup();

    return 0;
}