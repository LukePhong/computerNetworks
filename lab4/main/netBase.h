//
// Created by 'Confidence'F on 11/19/2022.
//

#ifndef TRY1_NETBASE_H
#define TRY1_NETBASE_H

#include "message.h"

class netBase {
private:

protected:
    static unsigned short checksum(char *msg, unsigned int bytecount);
    static bool checksumchecker(char *msg, unsigned int bytecount);
};


#endif //TRY1_NETBASE_H
