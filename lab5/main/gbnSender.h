//
// Created by 'Confidence'F on 12/27/2022.
//

#ifndef TRY1_GBNSENDER_H
#define TRY1_GBNSENDER_H

#include <queue>

using namespace std;
//#include <winsock.h>
#include "unifiedBlock.h"
#include "server.h"

class gbnSender : netBase {
private:
//    sockaddr_in &client;
    int dwSendSize;
    bool needResend = false;
    queue<char*> windowQueue;
    queue<long long> timeQueue;
    queue<const unifiedBlock*> wait4ack;
    size_t window_size = 32768;
    size_t base, next;
//    char buffertemp[4096];
    long long timeout = 3000;

    server& theServerToServe;

public:

    gbnSender(int dwSendSize, server &theServerToServe) : dwSendSize(dwSendSize), theServerToServe(theServerToServe) {};

    void send_gbn(const unifiedBlock *c);

    void baseAndNextToTheSameInitPlace();

    void resendEverythingInWindow();

    void windowAndWaitlistGoAroundOnce();

    char *getNextPackageOfListItemToSend(const unifiedBlock *listItem) const;

    static size_t getPackageSizeofDataAndTransHead(const unifiedBlock *listItem) ;

    static char *getSendBufferReady(const unifiedBlock *listItem, transmission &tran) ;

    const unifiedBlock *sendOneItem(const unifiedBlock *listItem);

    bool ableToSendNextItem(const unifiedBlock *listItem) const;

    size_t recieveAndWaitBaseMoveFront(char *bufferForReceive, const unifiedBlock *listItem);
};


#endif //TRY1_GBNSENDER_H
