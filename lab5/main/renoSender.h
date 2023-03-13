//
// Created by 'Confidence'F on 12/28/2022.
//

#ifndef TRY1_RENOSENDER_H
#define TRY1_RENOSENDER_H

#include <queue>
#include <iostream>
using namespace std;
#include <pthread.h>
//#include <winsock.h>
#include "unifiedBlock.h"
#include "netBase.h"
#include "server.h"

//class server;

class renoSender : public netBase {
private:
    bool needResend;
    bool shouldTerminate;
    struct windowItem{
        char* b;
        long long sentTime;
        unifiedBlock *item;
        bool onceResent = false;
    };
    queue<windowItem*> windowQueue;
    const size_t MSS = 1024;    // not include head
    size_t cwnd;
    size_t ssthresh;
    size_t volumeInWindow;
    size_t acked, beforeQueue;
    long long timeout;
    int dupACKCount;
    enum conditions {SLOW_START, CONGESTION_AVOIDANCE, FAST_RECOVERY};
    conditions current_cond = SLOW_START;
    server *theServerToServe;
    unifiedBlock *headToSend;
    windowItem *toResend;
    char* currentReceived;

    pthread_t sendThread, receiveThread, timerThread;
    pthread_mutex_t windowQueueMutex;
    pthread_mutex_t outputMutex;
//    pthread_mutex_t serverMutex;

public:
    renoSender(server *theServerToServe);

    void renoSend(unifiedBlock *c);

    void startAllThreads();

    void waitForAllThreadsExit();

    static void *run_send(void *arg);

    static void *run_receive(void *arg);

    static void *run_timer(void *arg);

    void resetArguments();

    void send();

    void receive();

    void timer();

    char *getNextPackageOfListItemToSend(const unifiedBlock *listItem) const;

    static size_t getPackageSizeofDataAndTransHead(const unifiedBlock *listItem) ;

    static char *getSendBufferReady(const unifiedBlock *listItem, transmission &tran) ;

    void sendNextPacketFromHead();

    bool isTimeout();

    bool isBroken(char *receive);

    bool isDupACK(char *receive);

    void updateBase(char *receive);

    bool needShrinkQueue();

    void changeCwndAndCondOnNewACK();

    void changeCwndAndOthersOnDupACK();

    void sendLogging();

    void resendLogging();

    void recieveLogging(char *buf, bool isDupACK);

    void timerLogging();

    void setNeedResendForDup();
};


#endif //TRY1_RENOSENDER_H
