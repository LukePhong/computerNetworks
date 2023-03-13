//
// Created by 'Confidence'F on 10/20/2022.
//

#ifndef PROJ1_SERVER_H
#define PROJ1_SERVER_H

#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <map>
using namespace std;

#include "prettyPrint.h"
#include "queue_block.h"
#include "message.h"
//#include "bean.h"


#define DEFAULT_BUFFER 4096 /*缓冲区大小*/

class server {
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
//    vector<BlockQueue*>* bqM2N;
    map<SOCKET, BlockQueue*>* bqM2N;

private:
    static message* extractHeader(char* buffer);
    static bool shouldExit;
public:
    server(unsigned short, map<SOCKET, BlockQueue*>* bqM2N);
    int startListening();
    int startMainLoop();
    static DWORD WINAPI ClientThread(LPVOID);
    int serverClose();
    static void* sendToClient(void* arg);

    static BlockQueue* bqN2M;
    static pthread_mutex_t bqM2NLock;
private:
    // for threading
    vector<pthread_t> sendThreadVec;
    static pthread_mutex_t& toMainLock;
private:
    struct channel {
        BlockQueue* in;
        SOCKET out;
    };
};


#endif //PROJ1_SERVER_H
