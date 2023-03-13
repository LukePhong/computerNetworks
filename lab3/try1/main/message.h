//
// Created by 'Confidence'F on 10/21/2022.
//

#ifndef PROJ1_MESSAGE_H
#define PROJ1_MESSAGE_H

#include <ctime>
#include <string>
using namespace std;

#pragma pack (1)
struct transmission{
    unsigned int seq;
    unsigned int ok_seq;
    char cond;
    unsigned short window;
    unsigned short checksum;
};
#pragma pack ()

#pragma pack (1)
struct fileInfo{
    char name[256];
    char type[8];
    unsigned int size;
};
#pragma pack ()

enum condition {
    ack = 1,
    syn = 2,
    fin = 4
};

class transmission_message{
public:
    static transmission pure_ack;
    static transmission pure_syn;
    static transmission syn_ack;
    static transmission fin_ack;
};


//enum opcode {
//    user_registration = 2,
//    join = 1,
//    plan_text = 0,
//    user_group_info = 3,
//    quit_group = 4,
//    program_exit = 254,
//    error = 255
//};

#define HEADER_LENGTH sizeof(struct message)

#endif //PROJ1_MESSAGE_H
