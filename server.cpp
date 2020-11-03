#include <stdio.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <errno.h>

int main() {
    WSADATA wsaData;    
    struct sockaddr_in addr;
    struct sockaddr_in client;
    
    //  (0) Initialize
    if (WSAStartup(MAKEWORD(2 ,0), &wsaData) == SOCKET_ERROR) {
        printf ("Error initialising WSA.\n");
        return -1;
    }

    //  (1) Open Socket
    SOCKET sock0 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock0 < 0) {
        perror("socket");
        printf("%d\n", errno);
        return 1;
    }
    printf("after scoket\n");

    //  (2) bind
    addr.sin_family = AF_INET;
    addr.sin_port = htons(50000);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock0, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind");
        return 1;
    }

    //  (3) listen
    if (listen(sock0, 5) != 0) {
        perror("listen");
        return 1;
    }

    //  (4) accept
    while (1) {
        int len = sizeof(client);
        SOCKET sock = accept(sock0, (struct sockaddr *)&client, &len);
        if (sock < 0) {
            perror("accept");
            break;
        }

        int n = send(sock, "RDCS SERVER", 11, 0);
        if (n < 1) {
            perror("write");
            break;
        }

        closesocket(sock);
    }
 
    //  (5) Terminate
    closesocket(sock0);
    WSACleanup();
    
    return 0;
}