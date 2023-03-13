//
// Created by 'Confidence'F on 12/30/2022.
//

#include <sstream>
#include "gbnRenoSender.h"

gbnRenoSender::gbnRenoSender(server *theServerToServe) : theServerToServe(theServerToServe) {}

void gbnRenoSender::renoSend(unifiedBlock *c) {
    if(!c)
        return;
    headToSend = c;
    resetArguments();
    startAllThreads();
    waitForAllThreadsExit();
}

void gbnRenoSender::startAllThreads() {

    pthread_mutex_init(&windowQueueMutex,NULL);
    pthread_mutex_init(&outputMutex,NULL);
//    pthread_mutex_init(&serverMutex,NULL);
    pthread_create(&sendThread, nullptr, run_send, this);
    pthread_create(&receiveThread, nullptr, run_receive, this);
    pthread_create(&timerThread, nullptr, run_timer, this);
}

void gbnRenoSender::waitForAllThreadsExit() {

    pthread_join(sendThread, nullptr);
    pthread_join(receiveThread, nullptr);
    pthread_join(timerThread, nullptr);
    pthread_mutex_destroy(&windowQueueMutex);
    pthread_mutex_destroy(&outputMutex);
//    pthread_mutex_destroy(&serverMutex);
    cout<<"all threads exited"<<endl;
}

void *gbnRenoSender::run_send(void *arg) {
    auto r = (gbnRenoSender*)arg;
    r->send();
    return nullptr;
}

void *gbnRenoSender::run_receive(void *arg) {
    auto r = (gbnRenoSender*)arg;
    r->receive();
    return nullptr;
}

void *gbnRenoSender::run_timer(void *arg) {
    auto r = (gbnRenoSender*)arg;
    r->timer();
    return nullptr;
}

void gbnRenoSender::resetArguments() {
    needResend = false;
    shouldTerminate = false;
    cwnd = MSS;
    ssthresh = 65536;
    timeout = 3000;
    dupACKCount = 0;
    volumeInWindow = 0;
    current_cond = SLOW_START;
    currResend = nullptr;
    currentReceived = nullptr;
    acked = theServerToServe->getCurrSeq();
    beforeQueue = theServerToServe->getCurrSeq();
}

void gbnRenoSender::send() {

    while(!shouldTerminate){
        if(needResend) {
            pthread_mutex_lock(&windowQueueMutex);
            resendEverythingInNoACKWindow();
            pthread_mutex_unlock(&windowQueueMutex);
        }
        pthread_mutex_lock(&windowQueueMutex);
        if(headToSend && volumeInWindow + headToSend->getSize() <= cwnd) {
            sendNextPacketFromHead();
        }
        pthread_mutex_unlock(&windowQueueMutex);
    }
}

void gbnRenoSender::resendEverythingInNoACKWindow() {
    size_t s = windowQueue.size();
    windowItem* toResend;
    for (int i = 0; i < s; ++i) {
        toResend = windowQueue.front();
        currResend = toResend;
        theServerToServe->sendPacket(toResend->b, getPackageSizeofDataAndTransHead(toResend->item));
        toResend->onceResent = true;
        toResend->sentTime = timeGetTime();
//        resendLogging();
        windowQueue.push(windowQueue.front());
        windowQueue.pop();
    }
    needResend = false;
}

void gbnRenoSender::timer() {
    while(!shouldTerminate){

        Sleep(timeout / 10);
        pthread_mutex_lock(&windowQueueMutex);
        if(isTimeout()){
            ssthresh = cwnd / 2;
            cwnd = MSS;
            dupACKCount = 0;
//            timerLogging();
//            toResend = windowQueue.front();
            needResend = true;
            current_cond = SLOW_START;
        }
        pthread_mutex_unlock(&windowQueueMutex);
    }
}

void gbnRenoSender::receive() {

    char bufferForReceive[4096];
    while(!shouldTerminate){
//        pthread_mutex_lock(&serverMutex);
        theServerToServe->receivePacket(bufferForReceive, 4096);
        currentReceived = bufferForReceive;
//        pthread_mutex_unlock(&serverMutex);
        if(isBroken(bufferForReceive)) {
            memset(bufferForReceive, 0, 4096);
            continue;
        }
        if(isDupACK(bufferForReceive)){
            pthread_mutex_lock(&windowQueueMutex);
            changeCwndAndOthersOnDupACK();
            pthread_mutex_unlock(&windowQueueMutex);
//            recieveLogging(bufferForReceive, true);
        }else{
            updateBase(bufferForReceive);
            changeCwndAndCondOnNewACK();
            pthread_mutex_lock(&windowQueueMutex);
            while(needShrinkQueue()) {
                if (!windowQueue.empty()) {
                    volumeInWindow -= windowQueue.front()->item->getSize();
                    beforeQueue += windowQueue.front()->item->getSize();
                    delete[] windowQueue.front()->b;
                    windowQueue.pop();
                }
                if (windowQueue.empty() && !headToSend) {
                    shouldTerminate = true;
                }
            }
            pthread_mutex_unlock(&windowQueueMutex);
//            recieveLogging(bufferForReceive, false);
        }
        if(cwnd >= ssthresh && current_cond == SLOW_START)
            current_cond = CONGESTION_AVOIDANCE;
    }
}

void gbnRenoSender::changeCwndAndOthersOnDupACK() {
    if(current_cond == FAST_RECOVERY){
        cwnd += MSS;
    }else {
        dupACKCount++;
        if (dupACKCount == 3) {
            ssthresh = cwnd / 2;
            cwnd = ssthresh + 3 * MSS;
            current_cond = FAST_RECOVERY;
//            setNeedResendForDup();
//            toResend = windowQueue.front();
            needResend = true;
        }
    }
}

void gbnRenoSender::changeCwndAndCondOnNewACK() {
    switch (current_cond) {
        case SLOW_START:
            cwnd += MSS;
            break;
        case CONGESTION_AVOIDANCE:
            cwnd += MSS * (1.0 * MSS / cwnd);
            break;
        case FAST_RECOVERY:
            cwnd = ssthresh;
            current_cond = CONGESTION_AVOIDANCE;
    }
    dupACKCount = 0;
}

size_t gbnRenoSender::getPackageSizeofDataAndTransHead(const unifiedBlock *listItem) { return sizeof(transmission) + listItem->getSize(); }

void gbnRenoSender::sendNextPacketFromHead() {
    if(!headToSend)
        return;

    char *s = getNextPackageOfListItemToSend(headToSend);
    auto wi = new windowItem{s, timeGetTime(), headToSend};
//    pthread_mutex_lock(&serverMutex);
    theServerToServe->sendPacket(wi->b, getPackageSizeofDataAndTransHead(wi->item));
//    pthread_mutex_unlock(&serverMutex);
//    pthread_mutex_lock(&windowQueueMutex);
    windowQueue.push(wi);
//    pthread_mutex_unlock(&windowQueueMutex);
    volumeInWindow += wi->item->getSize();
    headToSend = headToSend->getNext();
//    sendLogging();
}

char *gbnRenoSender::getNextPackageOfListItemToSend(const unifiedBlock *listItem) const {
    theServerToServe->updateCurrSeq(listItem->getSize());

    struct transmission tran = transmission_message::pure_ack;
    tran.seq = theServerToServe->getCurrSeq();
    tran.ok_seq = theServerToServe->getClientSeq();
//    tran.ok_seq = -1;

    theServerToServe->updateClientSeq();
    char *s = getSendBufferReady(listItem, tran);
    return s;
}

char *gbnRenoSender::getSendBufferReady(const unifiedBlock *listItem, transmission &tran) {
    auto s = new char[getPackageSizeofDataAndTransHead(listItem)];
    memcpy(s, &tran, sizeof(transmission));
    memcpy(s + sizeof(transmission), listItem->getBuffer(), listItem->getSize());
    ((transmission *) s)->checksum = checksum(s, sizeof(transmission) + listItem->getSize());
    return s;
}

bool gbnRenoSender::isTimeout() {

    if(windowQueue.empty())
        return false;

//    if(!windowQueue.front()->onceResent && timeGetTime() - windowQueue.front()->sentTime >= timeout)
    if(timeGetTime() - windowQueue.front()->sentTime >= timeout)
        return true;

    return false;
}

bool gbnRenoSender::isBroken(char *receive) {
    if(checksumchecker(receive, 0))
        return false;
    return true;
}

bool gbnRenoSender::isDupACK(char *receive) {
    auto t = (transmission*)receive;
    if(t->ok_seq > acked)
        return false;
    return true;
}

void gbnRenoSender::updateBase(char *receive) {
    auto t = (transmission*)receive;
    acked = t->ok_seq;
}

bool gbnRenoSender::needShrinkQueue() {
    if(acked > beforeQueue)
        return true;
    return false;
}

void gbnRenoSender::sendLogging() {
    stringstream ss;
    auto t = (transmission*)windowQueue.back()->b;
    ss<<"发送数据包序列号： "<<t->seq<<" 当前窗口大小(Byte)："<<cwnd<<" 窗口中未确认数据量(Byte)："<<volumeInWindow<<" ssthresh："<<ssthresh;
    pthread_mutex_lock(&outputMutex);
    theServerToServe->getPretty()->info(ss.str());
    pthread_mutex_unlock(&outputMutex);
}

void gbnRenoSender::resendLogging() {
    stringstream ss;
    auto t = (transmission*)currResend->b;
    ss<<"重新发送数据包序列号： "<<t->seq<<" 当前窗口大小(Byte)："<<cwnd<<" 窗口中未确认数据量(Byte)："<<volumeInWindow<<" ssthresh："<<ssthresh;
    pthread_mutex_lock(&outputMutex);
    theServerToServe->getPretty()->info(ss.str());
    pthread_mutex_unlock(&outputMutex);
}

void gbnRenoSender::recieveLogging(char *buf, bool isDupACK) {
    auto t = (transmission*)buf;
    stringstream ss;
    if(isDupACK){
        ss<<"收到重复ACK，确认号： "<<t->ok_seq;
    }else{
        ss<<"收到新的ACK，确认号： "<<t->ok_seq<<" 调整后窗口大小(Byte)："<<cwnd<<" 窗口中未确认数据量(Byte)："<<volumeInWindow<<" ssthresh："<<ssthresh;
    }
    pthread_mutex_lock(&outputMutex);
    theServerToServe->getPretty()->info(ss.str());
    pthread_mutex_unlock(&outputMutex);
}

void gbnRenoSender::timerLogging() {
    stringstream ss;
    auto t = (transmission*)windowQueue.front()->b;
    ss<<"发生了超时！超时序列号："<<t->seq<<" 调整后窗口大小(Byte)："<<cwnd<<" 窗口中未确认数据量(Byte)："<<volumeInWindow<<" ssthresh："<<ssthresh;
    pthread_mutex_lock(&outputMutex);
    theServerToServe->getPretty()->info(ss.str());
    pthread_mutex_unlock(&outputMutex);
}

//void gbnRenoSender::setNeedResendForDup() {
//    if(!currentReceived || windowQueue.empty())
//        return;
//    auto t = (transmission*)currentReceived;
//    size_t s = windowQueue.size();
//    for (int i = 0; i < s; ++i) {
//        auto c = (transmission*)windowQueue.front()->b;
//        if(c->seq == t->seq){
//            toResend = windowQueue.front();
//        }
//        windowQueue.push(windowQueue.front());
//        windowQueue.pop();
//    }
//}

