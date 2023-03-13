//
// Created by 'Confidence'F on 12/27/2022.
//

#include <cassert>
#include "gbnSender.h"

void gbnSender::send_gbn(const unifiedBlock *blockList) {

    baseAndNextToTheSameInitPlace();

    char bufferForReceive[4096];
    const auto* listItem = blockList;

    while(1){
        if(needResend){
            resendEverythingInWindow();
        }else if(listItem){
            listItem = sendOneItem(listItem);

            if(ableToSendNextItem(listItem))
                continue;
        }

        size_t diff = recieveAndWaitBaseMoveFront(bufferForReceive, listItem);

        size_t acked = 0;
        if(diff > 0){
            while(acked < diff){
                acked += wait4ack.front()->getSize();
                wait4ack.pop();
                timeQueue.pop();
                assert(windowQueue.front() != nullptr);
                delete[] windowQueue.front();
                windowQueue.pop();
            }
        }

        if(timeGetTime() - timeQueue.front() > timeout){
            needResend = true;
            cout<<"发现超时"<<endl;
        }

        if (listItem == nullptr && wait4ack.empty()){
            assert(windowQueue.empty());
            assert(timeQueue.empty());
            break;
        }
    }

    cout<<"发送结束"<<endl;
}

size_t gbnSender::recieveAndWaitBaseMoveFront(char *bufferForReceive, const unifiedBlock *listItem) {
    size_t t, diff;
    transmission *p;
    while(1) {
        theServerToServe.receivePacket(bufferForReceive, 4096);
        p = (transmission*)bufferForReceive;
        t = p->ok_seq;
        diff = t - base;
        //防止最后出现丢包
        if (p->cond == ack && (diff > 0 || listItem == nullptr) && checksumchecker(bufferForReceive, 0)){
            //client_seq = p->seq + 1;
            break;
        }
    }
    base = t;
    theServerToServe.getPretty()->outputBaseNext(static_cast<condition>(p->cond), timeGetTime(), p->seq, checksum(bufferForReceive, sizeof(transmission)),
                                                 base, next);
    return diff;
}

bool gbnSender::ableToSendNextItem(const unifiedBlock *listItem) const { return listItem && next < base + window_size; }

const unifiedBlock *gbnSender::sendOneItem(const unifiedBlock *listItem) {
    char *s = getNextPackageOfListItemToSend(listItem);

    windowQueue.push(s);

    theServerToServe.sendPacket(s, getPackageSizeofDataAndTransHead(listItem));

    timeQueue.push(timeGetTime());

    wait4ack.push(listItem);

    next += listItem->getSize();

    listItem = listItem->getNext();
    return listItem;
}

char *gbnSender::getNextPackageOfListItemToSend(const unifiedBlock *listItem) const {
    theServerToServe.updateCurrSeq(listItem->getSize());
    theServerToServe.updateClientSeq();

    struct transmission tran = transmission_message::pure_ack;
    tran.seq = theServerToServe.getCurrSeq();
    tran.ok_seq = theServerToServe.getClientSeq();

    char *s = getSendBufferReady(listItem, tran);
    return s;
}

char *gbnSender::getSendBufferReady(const unifiedBlock *listItem, transmission &tran) {
    auto s = new char[getPackageSizeofDataAndTransHead(listItem)];
    memcpy(s, &tran, sizeof(transmission));
    memcpy(s + sizeof(transmission), listItem->getBuffer(), listItem->getSize());
    ((transmission *) s)->checksum = checksum(s, sizeof(transmission) + listItem->getSize());
    return s;
}

size_t gbnSender::getPackageSizeofDataAndTransHead(const unifiedBlock *listItem) { return sizeof(transmission) + listItem->getSize(); }

void gbnSender::resendEverythingInWindow() {
    queue<long long>().swap(timeQueue); // clean time queue
    assert (windowQueue.size() == wait4ack.size());
    size_t queueLength = windowQueue.size();    //save the queue size
    theServerToServe.getPretty()->info("GBN进行超时重传");
    for (int i = 0; i < queueLength; i++) {
        theServerToServe.sendPacket(windowQueue.front(), sizeof(transmission) + wait4ack.front()->getSize());
        timeQueue.push(timeGetTime());  // note the time of sending
        windowAndWaitlistGoAroundOnce();
    }
    needResend = false;
}

void gbnSender::windowAndWaitlistGoAroundOnce() {
    windowQueue.push(windowQueue.front());
    windowQueue.pop();
    wait4ack.push(wait4ack.front());
    wait4ack.pop();
}

void gbnSender::baseAndNextToTheSameInitPlace() {
    base = theServerToServe.getCurrSeq();
    next = theServerToServe.getCurrSeq();
}
