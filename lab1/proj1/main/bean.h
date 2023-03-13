//
// Created by 'Confidence'F on 10/21/2022.
//

#ifndef PROJ1_BEAN_H
#define PROJ1_BEAN_H

#include <winsock.h>
#include "message.h"

struct bean{
    SOCKET socket;
    message m;
};



#endif //PROJ1_BEAN_H
