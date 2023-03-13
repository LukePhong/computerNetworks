//
// Created by 'Confidence'F on 11/17/2022.
//

#include<stdio.h>
#include<tchar.h>
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
using namespace std;


#pragma comment(lib, "ws2_32.lib")
int _tmain(int argc, _TCHAR* argv[])//_tmain,要加＃include <tchar.h>才能用
{
    WSAData wsd;           //初始化信息
    SOCKET soRecv;         //接收SOCKET
    char* pszRecv = NULL;  //接收数据的数据缓冲区指针
    int nRet = 0;
    //int i = 0;
    int dwSendSize = 0;
    SOCKADDR_IN siRemote, siLocal;    //远程发送机地址和本机接收机地址

    //启动Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        cout << "WSAStartup Error = " << WSAGetLastError() << endl;
        return 0;
    }
    else {
        cout << "start Success" << endl;
    }

    //创建socket
    soRecv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (soRecv == SOCKET_ERROR) {
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

    //绑定本地地址到socket
    if (bind(soRecv, (SOCKADDR*)&siLocal, sizeof(siLocal)) == SOCKET_ERROR) {
        cout << "bind Error = " << WSAGetLastError() << endl;
        return 1;
    }
    else {
        cout << "bind Success" << endl;
    }

    //申请内存
    pszRecv = new char[4096];
    if (pszRecv == NULL) {
        cout << "pszRecv new char Error " << endl;
        return 0;
    }
    else {
        cout << "pszRecv new char Success" << endl;
    }

    // 一直等待数据
    while(true){
        //for (int i = 0; i < 30; i++) {
        dwSendSize = sizeof(siRemote);
        cout << "开始等待数据..." << endl;
        //开始接受数据
        nRet = recvfrom(soRecv, pszRecv, 4096, 0, (SOCKADDR*)&siRemote, &dwSendSize);
        cout << "收到数据!!" << endl;
        if (nRet == SOCKET_ERROR) {
            cout << "recvfrom Error " << WSAGetLastError() << endl;
            break;
        }
        else if (nRet == 0) {
            break;
        }
        else {
            pszRecv[nRet] = '\0';
            char sendBuf[20] = { '\0' };
            inet_ntop(AF_INET, (void*)&siRemote.sin_addr, sendBuf, 16);
            cout << "IP地址: " << sendBuf << endl
                 << "数据: " << pszRecv << endl;
        }
    }
    //关闭socket连接
    closesocket(soRecv);
    delete[] pszRecv;

    //清理
    WSACleanup();
    system("pause");
    return 0;
}
