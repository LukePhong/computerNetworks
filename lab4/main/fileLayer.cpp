//
// Created by 'Confidence'F on 11/18/2022.
//

#include "fileLayer.h"

fileBlockHead *fileLayer::loadFile(string filePath) {


    auto filehead = new fileBlockHead();

    auto buff = new char[FILE_BLOCK_LIMIT];

    string fileName = filePath.substr(filePath.find_last_of('/') + 1, -1);
//    cout<<fileName <<endl;
//    struct _stat getinfo;
//    _stat(fileName.c_str(), &getinfo);

    struct fileInfo info;
    HANDLE handle = CreateFile(filePath.c_str(), FILE_READ_EA,
                               FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (handle != INVALID_HANDLE_VALUE) {
        info.size = GetFileSize(handle, NULL);
        CloseHandle(handle);
    }else{
        info.size = 0;
    }

    size_t pos = fileName.find(".");
    string name = fileName.substr(0, pos);
    string type = fileName.substr(pos + 1, -1);

    strcpy(info.name, name.c_str());
    strcpy(info.type, type.c_str());

//    cout << info.size << endl;

    filehead->setFilename(name);
    filehead->setType(type);
    filehead->setSize(info.size);

    ifstream open_file(filePath, std::ios::binary);
    if (!open_file) {
        std::cout << "error" << std::endl;
        return 0;
    }

    bool isFirst = true;
    memcpy(buff, &info, sizeof(fileInfo));
    while (!open_file.eof()) {
        if (isFirst) {
            open_file.read(buff + sizeof(fileInfo), FILE_BLOCK_LIMIT - sizeof(fileInfo));
            isFirst = false;
        } else {
            open_file.read(buff, FILE_BLOCK_LIMIT);
        }
        filehead->addBlock(buff, FILE_BLOCK_LIMIT);
//        cout<<filehead->getBlkNum()<<endl;
        memset(buff, 0, FILE_BLOCK_LIMIT);
    }
    open_file.close();

    return filehead;
}

void fileLayer::putFile(string filePath, fileBlockHead* head) {

    ofstream ofs(filePath + head->getFilename() + "." + head->getType(), ios::out | ios::binary);

    unifiedBlock* curr = head->getHead();
    //让最后一个进不来
    while(curr->getNext()){
        ofs.write(curr->getBuffer(), curr->getSize());
        curr = curr->getNext();
    }
    ofs.write(curr->getBuffer(), FILE_BLOCK_LIMIT - (head->getBlkNum() * FILE_BLOCK_LIMIT - sizeof(fileInfo) - head->getSize()));

    ofs.close();
}
