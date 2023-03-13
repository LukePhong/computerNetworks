//
// Created by 'Confidence'F on 11/18/2022.
//

#include <cstring>
#include "blockHead.h"

blockHead::blockHead() {}

blockHead::blockHead(unifiedBlock *head, unsigned int blkNum) : head(head), blkNum(blkNum) {}

unifiedBlock *blockHead::getHead() const {
    return head;
}

void blockHead::setHead(unifiedBlock *head) {
    blockHead::head = head;
}

unsigned int blockHead::getBlkNum() const {
    return blkNum;
}

void blockHead::setBlkNum(unsigned int blkNum) {
    blockHead::blkNum = blkNum;
}

void blockHead::addBlock(char *bufffer, unsigned int size) {
    unifiedBlock* lst = head;
    //一定要开辟新的内存空间！
    auto buf = new char[size];
    memcpy(buf, bufffer, sizeof(char) * size);

    if(lst != nullptr) {
        while (lst->getNext() != nullptr) {
            lst = lst->getNext();
        }
    }else{
        head = new unifiedBlock(size, buf);
        blkNum ++;
        return;
    }

    auto newBlock = new unifiedBlock(size, buf);
    lst->setNext(newBlock);
    blkNum++;
}

void blockHead::countBlocks() {
    if(head == nullptr)
        blkNum = 0;
    unsigned int i = 0;
    unifiedBlock* next = head->getNext();
    //要算上head指向的这个块
    i++;
    while(next){
        i++;
        next = next->getNext();
    }
    blkNum = i;
}
