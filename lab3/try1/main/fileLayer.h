//
// Created by 'Confidence'F on 11/18/2022.
//

#ifndef TRY1_FILELAYER_H
#define TRY1_FILELAYER_H

#include <io.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <Windows.h>
using namespace std;

#include "fileBlockHead.h"

class fileLayer {
public:
    static fileBlockHead* loadFile(string fileName);
    static void putFile(string filePath, fileBlockHead* head);
};


#endif //TRY1_FILELAYER_H
