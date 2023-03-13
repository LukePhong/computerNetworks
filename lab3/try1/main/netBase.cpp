//
// Created by 'Confidence'F on 11/19/2022.
//

#include "netBase.h"

unsigned short netBase::checksum(char *msg, unsigned int bytecount) {
    unsigned int sum = 0;
    auto * addr = (unsigned short *)msg;
    unsigned short word = 0;

    // add 16-bit by 16-bit
    while(bytecount > 1)
    {
        sum += *addr++;
        bytecount -= 2;
    }

    // Add left-over byte, if any
    if (bytecount > 0) {
        *(char *)(&word) = *(char *)addr;
        sum += word;
    }

    // Fold 32-bit sum to 16 bits
    while (sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    word = ~sum;

    return word;
}

bool netBase::checksumchecker(char *msg, unsigned int bytecount) {
    //输入的是输入缓冲区，bytecount是收到的信息的长度，不包括头部

    char* buf = new char[bytecount + sizeof(transmission)];
    auto t = (transmission*)msg;
    unsigned short ornChecksum = t->checksum;
    t->checksum = 0;
    memcpy(buf, msg, bytecount + sizeof(transmission));
    unsigned short newChecksum = checksum(buf, bytecount + sizeof(transmission));

    delete[] buf;

    if(ornChecksum == newChecksum)
        return true;

    return false;
}
