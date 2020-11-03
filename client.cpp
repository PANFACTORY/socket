//  Client side
#include <stdio.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define _POSIX_SOURCE  1    /* POSIX comliant source (POSIX)*/
#define BUFFSIZE       256
#define FALSE          0
#define TRUE           1

int main(int argc, char *argv[]) {
    WSADATA wsaData;
    SOCKET sock;
 
    struct sockaddr_in server;
 
    char buf[32];
    char *deststr;
    unsigned int **addrptr;

    //  (0) Initialize 
    if (argc != 2) {
        printf("Usage : %s dest\n", argv[0]);
        return 1;
    }
    deststr = argv[1];
 
    if (WSAStartup(MAKEWORD(2,0), &wsaData) == SOCKET_ERROR) {
        printf ("Error initialising WSA.\n");
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    //  (1)
    server.sin_family = AF_INET;
    server.sin_port = htons(50000);
    server.sin_addr.s_addr = inet_addr(deststr);

    //  (2)connect
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) != 0) {
        perror("connect");
        return 1;
    }

    //  (3)
    char vect[512]={0};
    int get=recv(sock,vect,512,0);
    printf("read=%s\n", vect);
 
    //  (4) Terminate 
    closesocket(sock);
    WSACleanup();

    return 0;
}