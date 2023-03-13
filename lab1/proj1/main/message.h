//
// Created by 'Confidence'F on 10/21/2022.
//

#ifndef PROJ1_MESSAGE_H
#define PROJ1_MESSAGE_H

#include <ctime>
#include <string>
using namespace std;

#pragma pack (1)
struct message{
    unsigned int opcode;
    char user_name[128];
    time_t time;
    unsigned int length;
    char *message;
};
#pragma pack ()

enum opcode {
    user_registration = 2,
    join = 1,
    plan_text = 0,
    user_group_info = 3,
    quit_group = 4,
    program_exit = 254,
    error = 255
};

#define HEADER_LENGTH sizeof(struct message)

#endif //PROJ1_MESSAGE_H
