#include <iostream>
#include <unistd.h>

using namespace std;

#include "server.h"
#include "client.h"
#include "fileLayer.h"
#include "fileBlockHead.h"

char* hostAddr = "127.0.0.1";

string filedir = "./testfile/";
string outputdir = "./recievefile/";

//void networks(int);

int main(int argc, char **argv) {

    //中文输出
    system("chcp 65001");

    int mode = atoi(argv[1]);

//    networks(mode);

    string inputString;

    if (mode == 0){
        //服务器行为
        server srv(hostAddr, 9999);
        srv.startListening();
        srv.startMainLoop();

        fileBlockHead* filehead;

        cin>>inputString;
        while(inputString != "break"){
            filehead = fileLayer::loadFile(filedir + inputString);
            srv.sendBlocks(filehead);
            cin>>inputString;
        }

        srv.waveGoodby();

        srv.serverClose();
    }else{
        client clt(hostAddr, 4000);
        clt.connectToServer();

//        fileBlockHead* filehead;

//        filehead = clt.recvBlocks();
//        fileLayer::putFile(outputdir, filehead);
        clt.recvBlocks(outputdir);

        clt.waveGoodby();

        clt.closeConnect();
    }

    return 0;
}


//void networks(int mode) {
//
//    // 1=客户端 0=服务器
//    if (mode == 1){
//        // client默认这里是0
//        // 指定服务器地址和端口号
//        client clt(hostAddr, 9999);
//        clt.connectToServer();
////        clt.closeConnect();
//    }else{
//        // 服务器端口号和控制-网络队列
//        server srv(9999);
//        srv.startListening();
//        srv.startMainLoop();
////        srv.serverClose();
//    }
//
//    return;
//}

