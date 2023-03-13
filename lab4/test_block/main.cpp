//
// Created by 'Confidence'F on 11/18/2022.
//

#include <io.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

using namespace std;

#include "blockHead.h"
#include "unifiedBlock.h"

string filedir = "./testfile";

void getAllFiles(const string& path, vector<string>& files) {
    //文件句柄
    long long hFile = 0;
    //文件信息
    struct _finddata_t fileinfo;
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1) {
        do {
            if ((fileinfo.attrib & _A_SUBDIR)) {  //比较文件类型是否是文件夹
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0) {
                    files.push_back(p.assign(path).append("\\").append(fileinfo.name));
                    //递归搜索
                    getAllFiles(p.assign(path).append("\\").append(fileinfo.name), files);
                }
            }
            else {
                files.push_back(p.assign(path).append("\\").append(fileinfo.name));
            }
        } while (_findnext(hFile, &fileinfo) == 0);  //寻找下一个，成功返回0，否则-1
        _findclose(hFile);
    }
}

int main(){

    vector<string> filenames;
    getAllFiles(filedir, filenames);
    vector<blockHead> heads;
    for (const auto& file : filenames){
        cout << file << endl;

        ifstream open_file(file, std::ios::binary);
        if (!open_file)
        {
            std::cout << "error" << std::endl;
            return 0;
        }

        heads.emplace_back();

        auto buff = new char[FILE_BLOCK_LIMIT];
        while(!open_file.eof()){
            open_file.read(buff, FILE_BLOCK_LIMIT);
            heads.back().addBlock(buff, FILE_BLOCK_LIMIT);
//            cout<<heads.back().getBlkNum()<<endl;
            memset(buff, 0, FILE_BLOCK_LIMIT);
        }
        open_file.close();
    }

    // test splitting
    for (auto& h:heads) {
        cout<<"number before: "<<h.getBlkNum()<<endl;
        auto toSplit = h.getHead();
        auto toSplitNext = h.getHead()->getNext();
        for (int i = 0; i < h.getBlkNum(); ++i) {
            unifiedBlock::splitBlock(toSplit, NET_BLOCK_LIMIT);
            toSplit = toSplitNext;
            if(toSplitNext)
                toSplitNext = toSplitNext->getNext();
        }
        h.countBlocks();
        cout<<"number now: "<<h.getBlkNum()<<endl;
    }

    // test merging
    cout<<"test merging"<<endl;
    for (auto& h:heads) {
        cout<<"number before: "<<h.getBlkNum()<<endl;
        auto toMerge = h.getHead();
        for (int i = 0; i < h.getBlkNum() / 4 ; i++) {
            unifiedBlock::mergeBlock(toMerge, 4, FILE_BLOCK_LIMIT);
            toMerge = toMerge->getNext();
        }
        h.countBlocks();
        cout<<"number now: "<<h.getBlkNum()<<endl;
    }


}
