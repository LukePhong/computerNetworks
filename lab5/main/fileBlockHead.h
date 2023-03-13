//
// Created by 'Confidence'F on 11/18/2022.
//

#ifndef TRY1_FILEBLOCKHEAD_H
#define TRY1_FILEBLOCKHEAD_H

#include "blockHead.h"
#include "message.h"

class fileBlockHead : public blockHead{
private:
    string filename;
    string type;
    unsigned int size;
public:
    fileBlockHead();

    const string &getFilename() const;

    void setFilename(const string &filename);

    const string &getType() const;

    void setType(const string &type);

    unsigned int getSize() const;

    void setSize(unsigned int size);
};


#endif //TRY1_FILEBLOCKHEAD_H
