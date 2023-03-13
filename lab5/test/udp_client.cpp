//
// Created by 'Confidence'F on 11/17/2022.
//
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")
int _tmain(int argc, _TCHAR* argv[])
{
    cout << "hello world" << endl;
    WSAData wsd;           //初始化信息
    SOCKET soSend;         //发送到的目的SOCKET
    int nRet = 0;
    int dwSendSize = 0;
    SOCKADDR_IN siLocal;    //本机接收机地址

    //启动Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {/*进行WinSocket的初始化,
        windows 初始化socket网络库，申请2，2的版本，windows socket编程必须先初始化。*/
        cout << "WSAStartup Error = " << WSAGetLastError() << endl;
        return 0;
    }
    else {
        cout << "WSAStartup Success" << endl;
    }

    //创建socket

    //AF_INET 协议族:决定了要用ipv4地址（32位的）与端口号（16位的）的组合
    //SOCK_DGRAM --  UDP类型，不保证数据接收的顺序，非可靠连接；
    soSend = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (soSend == SOCKET_ERROR) {
        cout << "socket Error = " << WSAGetLastError() << endl;
        return 1;
    }
    else {
        cout << "socket Success" << endl;
    }

    //设置端口号
    int nPort = 5150;
    siLocal.sin_family = AF_INET;
    siLocal.sin_port = htons(nPort);
    //siLocal.sin_addr.s_addr = inet_addr("127.0.0.1");
    inet_pton(AF_INET, "127.0.0.1", (void*)&siLocal.sin_addr.s_addr);


    for (int i = 0; i < 30; i++) {
        //开始发送数据
        //发送数据到指定的IP地址和端口
        nRet = sendto(soSend, "123 mutouren", strlen("123 mutouren"), 0, (SOCKADDR*)&siLocal, sizeof(SOCKADDR));
        if (nRet == SOCKET_ERROR) {
            cout << "sendto Error " << WSAGetLastError() << endl;
            break;
        }
        else {
            cout << "sendto Success" << endl;
        }
    }
    //关闭socket连接
    closesocket(soSend);
    //清理
    WSACleanup();

    return 0;
}
