//
// Created by 'Confidence'F on 11/18/2022.
//

#include "fileBlockHead.h"

fileBlockHead::fileBlockHead() {}

const string &fileBlockHead::getFilename() const {
    return filename;
}

void fileBlockHead::setFilename(const string &filename) {
    fileBlockHead::filename = filename;
}

const string &fileBlockHead::getType() const {
    return type;
}

void fileBlockHead::setType(const string &type) {
    fileBlockHead::type = type;
}

unsigned int fileBlockHead::getSize() const {
    return size;
}

void fileBlockHead::setSize(unsigned int size) {
    fileBlockHead::size = size;
}
