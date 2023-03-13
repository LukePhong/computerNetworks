//
// Created by 'Confidence'F on 11/18/2022.
//

#include <cstring>
#include "unifiedBlock.h"

unsigned int unifiedBlock::getSize() const {
    return size;
}

void unifiedBlock::setSize(unsigned int size) {
    unifiedBlock::size = size;
}

char *unifiedBlock::getBuffer() const {
    return buffer;
}

void unifiedBlock::setBuffer(char *buffer) {
    unifiedBlock::buffer = buffer;
}

unifiedBlock *unifiedBlock::getNext() const {
    return next;
}

void unifiedBlock::setNext(unifiedBlock *next) {
    unifiedBlock::next = next;
}

unifiedBlock::unifiedBlock(unsigned int size, char *buffer, unifiedBlock *prev) : size(size), buffer(buffer), prev(prev){
    prev->next = this;
}

unifiedBlock::unifiedBlock(unsigned int size, char *buffer) : size(size), buffer(buffer) {}

void unifiedBlock::splitBlock(unifiedBlock *b, unsigned int maxSize) {
    //划分block，使其每一块的大小都小于maxSize
    if(b->getSize() <= maxSize)
        return;
    unsigned int i = 0;
    unsigned int ornSize = b->getSize();
    auto oldBuf = b->getBuffer();
    auto currBlock = b;
    auto ornNext = b->getNext();
    char* newBuf;
    while(i < ornSize){
        if(ornSize - i >= maxSize) {
            newBuf = new char[maxSize];
            memcpy(newBuf, oldBuf, maxSize);
            oldBuf += maxSize;
        }
        else {
            newBuf = new char[ornSize - i];
            memcpy(newBuf, oldBuf, ornSize - i);
        }
        i += maxSize;
        currBlock->setBuffer(newBuf);
        currBlock->setSize(maxSize);
        auto newBlock = new unifiedBlock();
        newBlock->setPrev(currBlock);
        currBlock->setNext(newBlock);
        currBlock = newBlock;
    }
    currBlock->getPrev()->setNext(ornNext);
    delete currBlock;
}

unifiedBlock::~unifiedBlock() {
    delete[] buffer;
}

unifiedBlock::unifiedBlock(const unifiedBlock &b) {
    size = b.size;
//    buffer = b.buffer;
    memcpy(buffer, b.buffer, size * sizeof(char));
    prev = b.prev;
    next = b.next;
}

unifiedBlock *unifiedBlock::getPrev() const {
    return prev;
}

void unifiedBlock::setPrev(unifiedBlock *prev) {
    unifiedBlock::prev = prev;
}

void unifiedBlock::mergeBlock(unifiedBlock *b, int mergeNum, unsigned int maxSize) {
    unsigned int processedSize = b->getSize();
    if(processedSize >= maxSize)
        return;

    //保留原有buffer指针，并开辟新的内存空间
    auto buf = new char[maxSize];
    char* ornBuf = b->getBuffer();
    b->setBuffer(buf);
    memcpy(buf, ornBuf, processedSize);
    b->setSize(processedSize);
    unifiedBlock* next = b->getNext();
    //循环拷贝数据，注意退出条件和更新块大小属性
    while(processedSize < maxSize && next && processedSize + next->getSize() <= maxSize){
        memcpy(buf + processedSize, next->getBuffer(), next->getSize());
        processedSize += next->getSize();
        b->setSize(processedSize);
        next = next->getNext();
    }
    //更新next指针
    b->setNext(next);
}
