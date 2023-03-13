//
// Created by 'Confidence'F on 10/20/2022.
//

#ifndef PROJ1_CLIENT_H
#define PROJ1_CLIENT_H
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "prettyPrint.h"
//#include "queue_block.h"
#include "message.h"
#include "fileBlockHead.h"
#include "netBase.h"

#define DEFAULT_BUFFER 4096 /*缓冲区大小*/

class client : netBase {
private:
    WSADATA wsd;
    static SOCKET sClient;
    static char Buffer[DEFAULT_BUFFER];
    struct sockaddr_in server;
    unsigned short port;
    struct hostent *host = NULL;
    unsigned int curr_seq = 0;
    unsigned int server_seq = 0;
//    unsigned short boundport;
//    struct sockaddr_in local;

    static bool shouldExit;

    prettyPrint* pretty;
public:
    client(char*, unsigned short);
//    client(char*, unsigned short, unsigned short);
    int connectToServer();
    int closeConnect();

//    fileBlockHead* recvBlocks();
    void recvBlocks(string);
//    int sendAndReceive();
//    static void* sender(void*);
//    static void* receiver(void*);

//    static BlockQueue* bqM2N;
    bool waveGoodby();

private:
//    static message* formMessage(char* Buffer);
//    static message* extractHeader(char* buffer);
    bool triHandshake();
    void sendrdt(char* b, unsigned int size);
    char* recvrdt(condition wanted, bool needNetInfo = false);

};


#endif //PROJ1_CLIENT_H
