//
// Created by 'Confidence'F on 10/20/2022.
//

#ifndef PROJ1_SERVER_H
#define PROJ1_SERVER_H

#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <iostream>
#include <Windows.h>
#include <queue>
using namespace std;

#include "prettyPrint.h"
#include "message.h"
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
    void send_gbn(const unifiedBlock *);
    void send_gbn_timer(const unifiedBlock *);
    void sendrdt(char* b, unsigned int size);
    char* recvrdt(condition wanted, bool needNetInfo = false);

public:
    server(char*, unsigned short);
    int startListening();
    int startMainLoop();
    int serverClose();
    void sendBlocks(blockHead* head);
    bool waveGoodby();

public:
    unsigned int getCurrSeq() const;

    unsigned int getClientSeq() const;

    void updateCurrSeq(unsigned int length);

    void updateClientSeq();

    prettyPrint *getPretty() const;

    bool sendPacket(char *buf, int len);

    bool receivePacket(char *buf, int len);
};


#endif //PROJ1_SERVER_H

