//
// Created by 'Confidence'F on 12/28/2022.
//

#include <sstream>
#include "renoSender.h"

renoSender::renoSender(server *theServerToServe) : theServerToServe(theServerToServe) {}

void renoSender::renoSend(unifiedBlock *c) {
    if(!c)
        return;
    headToSend = c;
    resetArguments();
    startAllThreads();
    waitForAllThreadsExit();
}

void renoSender::startAllThreads() {

    pthread_mutex_init(&windowQueueMutex,NULL);
    pthread_mutex_init(&outputMutex,NULL);
//    pthread_mutex_init(&serverMutex,NULL);
    pthread_create(&sendThread, nullptr, run_send, this);
    pthread_create(&receiveThread, nullptr, run_receive, this);
    pthread_create(&timerThread, nullptr, run_timer, this);
}

void renoSender::waitForAllThreadsExit() {

    pthread_join(sendThread, nullptr);
    pthread_join(receiveThread, nullptr);
    pthread_join(timerThread, nullptr);
    pthread_mutex_destroy(&windowQueueMutex);
    pthread_mutex_destroy(&outputMutex);
//    pthread_mutex_destroy(&serverMutex);
    cout<<"all threads exited"<<endl;
}

void *renoSender::run_send(void *arg) {
    auto r = (renoSender*)arg;
    r->send();
    return nullptr;
}

void *renoSender::run_receive(void *arg) {
    auto r = (renoSender*)arg;
    r->receive();
    return nullptr;
}

void *renoSender::run_timer(void *arg) {
    auto r = (renoSender*)arg;
    r->timer();
    return nullptr;
}

void renoSender::resetArguments() {
    needResend = false;
    shouldTerminate = false;
    cwnd = MSS;
    ssthresh = 65536;
    timeout = 3000;
    dupACKCount = 0;
    volumeInWindow = 0;
    current_cond = SLOW_START;
    toResend = nullptr;
    currentReceived = nullptr;
    acked = theServerToServe->getCurrSeq();
    beforeQueue = theServerToServe->getCurrSeq();
}

void renoSender::send() {

    while(!shouldTerminate){
        if(needResend) {
            pthread_mutex_lock(&windowQueueMutex);
//            pthread_mutex_lock(&serverMutex);
            theServerToServe->sendPacket(toResend->b, getPackageSizeofDataAndTransHead(toResend->item));
//            pthread_mutex_unlock(&serverMutex);
            toResend->onceResent = true;
            toResend->sentTime = timeGetTime();
            resendLogging();
            pthread_mutex_unlock(&windowQueueMutex);
            toResend = nullptr;
            needResend = false;
        }
        pthread_mutex_lock(&windowQueueMutex);
        if(volumeInWindow < cwnd) {
            sendNextPacketFromHead();
        }
        pthread_mutex_unlock(&windowQueueMutex);
    }
}

void renoSender::timer() {
    while(!shouldTerminate){

        Sleep(timeout / 10);
        pthread_mutex_lock(&windowQueueMutex);
        if(isTimeout()){
            ssthresh = cwnd / 2;
            cwnd = MSS;
            dupACKCount = 0;
            timerLogging();
            toResend = windowQueue.front();
            needResend = true;
            current_cond = SLOW_START;
        }
        pthread_mutex_unlock(&windowQueueMutex);
    }
}

void renoSender::receive() {

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
                recieveLogging(bufferForReceive, true);
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
            recieveLogging(bufferForReceive, false);
        }
        if(cwnd >= ssthresh && current_cond == SLOW_START)
            current_cond = CONGESTION_AVOIDANCE;
    }
}

void renoSender::changeCwndAndOthersOnDupACK() {
    if(current_cond == FAST_RECOVERY){
        cwnd += MSS;
    }else {
        dupACKCount++;
        if (dupACKCount == 3) {
            ssthresh = cwnd / 2;
            cwnd = ssthresh + 3 * MSS;
            current_cond = FAST_RECOVERY;
//            setNeedResendForDup();
            toResend = windowQueue.front();
            needResend = true;
        }
    }
}

void renoSender::changeCwndAndCondOnNewACK() {
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

size_t renoSender::getPackageSizeofDataAndTransHead(const unifiedBlock *listItem) { return sizeof(transmission) + listItem->getSize(); }

void renoSender::sendNextPacketFromHead() {
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
    sendLogging();
}

char *renoSender::getNextPackageOfListItemToSend(const unifiedBlock *listItem) const {
    theServerToServe->updateCurrSeq(listItem->getSize());

    struct transmission tran = transmission_message::pure_ack;
    tran.seq = theServerToServe->getCurrSeq();
    tran.ok_seq = theServerToServe->getClientSeq();
//    tran.ok_seq = -1;

    theServerToServe->updateClientSeq();
    char *s = getSendBufferReady(listItem, tran);
    return s;
}

char *renoSender::getSendBufferReady(const unifiedBlock *listItem, transmission &tran) {
    auto s = new char[getPackageSizeofDataAndTransHead(listItem)];
    memcpy(s, &tran, sizeof(transmission));
    memcpy(s + sizeof(transmission), listItem->getBuffer(), listItem->getSize());
    ((transmission *) s)->checksum = checksum(s, sizeof(transmission) + listItem->getSize());
    return s;
}

bool renoSender::isTimeout() {

    if(windowQueue.empty())
        return false;

//    if(!windowQueue.front()->onceResent && timeGetTime() - windowQueue.front()->sentTime >= timeout)
    if(timeGetTime() - windowQueue.front()->sentTime >= timeout)
        return true;

    return false;
}

bool renoSender::isBroken(char *receive) {
    if(checksumchecker(receive, 0))
        return false;
    return true;
}

bool renoSender::isDupACK(char *receive) {
    auto t = (transmission*)receive;
    if(t->ok_seq > acked)
        return false;
    return true;
}

void renoSender::updateBase(char *receive) {
    auto t = (transmission*)receive;
    acked = t->ok_seq;
}

bool renoSender::needShrinkQueue() {
    if(acked > beforeQueue)
        return true;
    return false;
}

void renoSender::sendLogging() {
    stringstream ss;
    auto t = (transmission*)windowQueue.back()->b;
    ss<<"发送数据包序列号： "<<t->seq<<" 当前窗口大小(Byte)："<<cwnd<<" 窗口中未确认数据量(Byte)："<<volumeInWindow<<" ssthresh："<<ssthresh;
    pthread_mutex_lock(&outputMutex);
    theServerToServe->getPretty()->info(ss.str());
    pthread_mutex_unlock(&outputMutex);
}

void renoSender::resendLogging() {
    stringstream ss;
    auto t = (transmission*)toResend->b;
    ss<<"重新发送数据包序列号： "<<t->seq<<" 当前窗口大小(Byte)："<<cwnd<<" 窗口中未确认数据量(Byte)："<<volumeInWindow<<" ssthresh："<<ssthresh;
    pthread_mutex_lock(&outputMutex);
    theServerToServe->getPretty()->info(ss.str());
    pthread_mutex_unlock(&outputMutex);
}

void renoSender::recieveLogging(char *buf, bool isDupACK) {
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

void renoSender::timerLogging() {
    stringstream ss;
    auto t = (transmission*)windowQueue.front()->b;
    ss<<"发生了超时！超时序列号："<<t->seq<<" 调整后窗口大小(Byte)："<<cwnd<<" 窗口中未确认数据量(Byte)："<<volumeInWindow<<" ssthresh："<<ssthresh;
    pthread_mutex_lock(&outputMutex);
    theServerToServe->getPretty()->info(ss.str());
    pthread_mutex_unlock(&outputMutex);
}

void renoSender::setNeedResendForDup() {
    if(!currentReceived || windowQueue.empty())
        return;
    auto t = (transmission*)currentReceived;
    size_t s = windowQueue.size();
    for (int i = 0; i < s; ++i) {
        auto c = (transmission*)windowQueue.front()->b;
        if(c->seq == t->seq){
            toResend = windowQueue.front();
        }
        windowQueue.push(windowQueue.front());
        windowQueue.pop();
    }
}
