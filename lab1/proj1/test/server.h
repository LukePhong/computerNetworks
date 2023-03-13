//
// Created by 'Confidence'F on 10/20/2022.
//

#ifndef PROJ1_SERVER_H
#define PROJ1_SERVER_H

#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_BUFFER 4096 /*缓冲区大小*/


int main_server(int argc, char **argv);

#endif //PROJ1_SERVER_H
