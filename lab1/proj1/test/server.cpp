#include "server.h"


/*与客户机通信的线程函数*/
DWORD WINAPI ClientThread(LPVOID lpParam) {
//void* ClientThread(void * lpParam) {
    SOCKET sock = (SOCKET) lpParam;
    char Buffer[DEFAULT_BUFFER];
    int ret, nLeft, idx;
    while (1) {
        /*接收来自客户机的消息*/
        ret = recv(sock, Buffer, DEFAULT_BUFFER, 0);
        if (ret == 0)
            break;
        else if (ret == SOCKET_ERROR) {
            printf("recv() 失败: %d\n", WSAGetLastError());
            break;
        }
        Buffer[ret] = '\0';
        printf("Recv: %s\n", Buffer);    //打印接收到的消息


        nLeft = ret;
        idx = 0;
        while (nLeft > 0) {
            /*向客户机发送回应消息*/
            ret = send(sock, &Buffer[idx], nLeft, 0);
            if (ret == 0)
                break;
            else if (ret == SOCKET_ERROR) {
                printf("send() 失败: %d\n", WSAGetLastError());
                break;
            }
            nLeft -= ret;
            idx += ret;
        }
    }
    return 0;
}

int main_server(int argc, char **argv) {
    WSADATA wsd;
    HANDLE hThread;
    DWORD dwThread;
    SOCKET sListen, sClient;
    int AddrSize;
    unsigned short port;
    struct sockaddr_in local, client;

    if (argc < 2) {
        printf("Usage:%s Port\n", argv[0]);
        return -1;
    }

    /*加载Winsock DLL*/
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("WinSock 初始化失败!\n");
        return 1;
    }

    /*创建Socket*/
    sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sListen == SOCKET_ERROR) {
        printf("socket() 失败: %d\n", WSAGetLastError());
        return 1;
    }

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    port = atoi(argv[1]);    //获取端口值
    local.sin_port = htons(port);

    /*绑定Socket*/
    if (bind(sListen,
             (struct sockaddr *) &local,
             sizeof(local)) == SOCKET_ERROR) {
        printf("bind() 失败: %d\n", WSAGetLastError());
        return 1;
    }
    /*打开监听*/
    listen(sListen, 8);

    /*在端口进行监听，一旦有客户机发起连接请示
     就建立与客户机进行通信的线程*/
    while (1) {
        AddrSize = sizeof(client);
        /*监听是否有连接请求*/
        sClient = accept(sListen, (struct sockaddr*)&client, &AddrSize);
        if (sClient == INVALID_SOCKET) {
            printf("accept() 失败: %d\n", WSAGetLastError());
            break;
        }
        printf("接受客户端连接: %s:%d\n",
               inet_ntoa(client.sin_addr),
               ntohs(client.sin_port));

        //创建一个线程去处理
        hThread = CreateThread(NULL, 0, ClientThread,
                               (LPVOID)sClient, 0, &dwThread);

//        pthread_t pid1;
//        int ret;
        if (hThread == NULL) {
//        if ((ret = pthread_create(&pid1, NULL, (void *)ClientThread, (void*)&sClient)) != 0){
            printf("CreateThread() 失败: %d\n", GetLastError());
            break;
        }
        //处理完后关闭
        CloseHandle(hThread);
    }
    closesocket(sListen);
    WSACleanup();    //用完了要清理
    return 0;
}