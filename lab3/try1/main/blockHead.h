//
// Created by 'Confidence'F on 11/18/2022.
//

#ifndef TRY1_BLOCKHEAD_H
#define TRY1_BLOCKHEAD_H

#include "unifiedBlock.h"

class blockHead {
private:
    unifiedBlock* head = nullptr;
    unsigned int blkNum = 0;
public:
    blockHead();

    blockHead(unifiedBlock *head, unsigned int blkNum);

    unifiedBlock *getHead() const;

    void setHead(unifiedBlock *head);

    unsigned int getBlkNum() const;

    void setBlkNum(unsigned int blkNum);

    void addBlock(char*, unsigned int);

    void countBlocks();
};


#endif //TRY1_BLOCKHEAD_H
