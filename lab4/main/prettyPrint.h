//
// Created by 'Confidence'F on 10/21/2022.
//

#ifndef PROJ1_PRETTYPRINT_H
#define PROJ1_PRETTYPRINT_H

#include <iostream>
using namespace std;

//#include "queue_block.h"
//#include "bean.h"
#include "message.h"

class prettyPrint {
private:
    long long timelast;

public:
//    static void pretty(Task t);
//    static void pretty(bean b);
    prettyPrint(long long int timelast);

    void pretty(condition c, long long currtime, unsigned int seq, unsigned short checksum);
    void outputSpeed(long long currtime, long long starttime, unsigned int size);
    void outputBaseNext(condition c, long long currtime, unsigned int seq, unsigned short checksum, size_t base, size_t next);
};


#endif //PROJ1_PRETTYPRINT_H
