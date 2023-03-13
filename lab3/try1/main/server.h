//
// Created by 'Confidence'F on 10/20/2022.
//

#ifndef PROJ1_SERVER_H
#define PROJ1_SERVER_H

#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
//#include <pthread.h>
#include <vector>
#include <map>
#include <iostream>
#include <Windows.h>
//#include <WS2tcpip.h>
using namespace std;

#include "prettyPrint.h"
//#include "queue_block.h"
#include "message.h"
//#include "bean.h"
#include "fileBlockHead.h"
#include "netBase.h"

#define DEFAULT_BUFFER 4096 /*缓冲区大小*/

class server : netBase {
private:
    WSADATA wsd;
    vector<HANDLE> hThreadVec;
    vector<DWORD> dwThreadVec;
    SOCKET sListen;
    SOCKET sClient;
    int AddrSize = sizeof(sockaddr_in);
    unsigned short port;
    struct sockaddr_in local;
    vector<sockaddr_in> clientVec;
    unsigned int curr_seq = 0;
    unsigned int client_seq = 0;

    //是否是向接收ack0那半边去
    bool toZero = true;

    prettyPrint* pretty;
private:
    static bool shouldExit;

private:
    bool triHandshake(char* buffer);

public:
//    server(unsigned short, map<SOCKET, BlockQueue*>* bqM2N);
    server(char*, unsigned short);
    int startListening();
    int startMainLoop();
//    static DWORD WINAPI ClientThread(LPVOID);
    int serverClose();
//    static void* sendToClient(void* arg);

//    static BlockQueue* bqN2M;

    void sendBlocks(blockHead* head);
    bool waveGoodby();
private:
    void sendrdt(char* b, unsigned int size);
    char* recvrdt(condition wanted, bool needNetInfo = false);
};


#endif //PROJ1_SERVER_H

