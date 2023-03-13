//
// Created by 'Confidence'F on 11/18/2022.
//

#ifndef TRY1_UNIFIEDBLOCK_H
#define TRY1_UNIFIEDBLOCK_H

#define FILE_BLOCK_LIMIT 4096
#define NET_BLOCK_LIMIT 1024

class unifiedBlock {
private:
    unsigned int size;
    char* buffer;
    unifiedBlock* next = nullptr;
    unifiedBlock* prev = nullptr;
public:
    unifiedBlock() = default;

    unifiedBlock(unsigned int size, char *buffer);

    // 注意！prev不能为空
    unifiedBlock(unsigned int size, char* buffer, unifiedBlock *prev);

    unifiedBlock(unifiedBlock const &);

    virtual ~unifiedBlock();

    unsigned int getSize() const;

    void setSize(unsigned int size);

    char *getBuffer() const;

    void setBuffer(char *buffer);

    unifiedBlock *getNext() const;

    void setNext(unifiedBlock *next);

    unifiedBlock *getPrev() const;

    void setPrev(unifiedBlock *prev);

public:
    static void splitBlock(unifiedBlock* b, unsigned int maxSize);
    static void mergeBlock(unifiedBlock* b, int mergeNum, unsigned int maxSize);
};


#endif //TRY1_UNIFIEDBLOCK_H
