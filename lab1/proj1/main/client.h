//
// Created by 'Confidence'F on 10/20/2022.
//

#ifndef PROJ1_CLIENT_H
#define PROJ1_CLIENT_H
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prettyPrint.h"
#include "queue_block.h"
//#include "message.h"

#define DEFAULT_BUFFER 4096 /*缓冲区大小*/

class client {
private:
    WSADATA wsd;
    static SOCKET sClient;
    static char Buffer[DEFAULT_BUFFER];
    struct sockaddr_in server;
    unsigned short port;
    struct hostent *host = NULL;

    static bool shouldExit;
public:
    client(char*, unsigned short);
    int connectToServer();
    int closeConnect();
    int sendAndReceive();
    static void* sender(void*);
    static void* receiver(void*);

    static BlockQueue* bqM2N;
private:
    static message* formMessage(char* Buffer);
    static message* extractHeader(char* buffer);
};


#endif //PROJ1_CLIENT_H
