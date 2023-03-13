//
// Created by 'Confidence'F on 10/20/2022.
//

#include <cmath>
#include <cassert>
#include "client.h"
#include "fileLayer.h"

char client::Buffer[];
SOCKET client::sClient;
//BlockQueue* client::bqM2N;

bool client::shouldExit = false;

//client::client(char* hostAddress, unsigned short port, unsigned short boundport) : port(port), boundport(boundport) {
client::client(char *hostAddress, unsigned short port) : port(port) {
    /*加载Winsock DLL*/
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("Winsock    初始化失败!\n");
        return;
    }

    /*创建Socket*/
    sClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sClient == INVALID_SOCKET) {
        printf("socket() 失败: %d\n", WSAGetLastError());
        return;
    }

    /*指定服务器地址*/
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(hostAddress);

    if (server.sin_addr.s_addr == INADDR_NONE) {
        host = gethostbyname(hostAddress);    //输入的地址可能是域名等
        if (host == NULL) {
            printf("无法解析服务端地址: %s\n", hostAddress);
            return;
        }
        CopyMemory(&server.sin_addr,
                   host->h_addr_list[0],
                   host->h_length);
    }

    pretty = new prettyPrint(timeGetTime());
}

bool client::triHandshake() {
    //三次握手建立连接
    char buffertemp[4096];
    int dwSendSize = sizeof(server);
    int nRet = 0;

    auto m1 = transmission_message::pure_syn;
    memcpy(buffertemp, &m1, sizeof(transmission));
    nRet = sendto(sClient, buffertemp, sizeof(transmission), 0, (SOCKADDR *) &server, sizeof(SOCKADDR));
    curr_seq++;
    cout << "成功发送syn" << endl;
    if (nRet == SOCKET_ERROR) {
        cout << "sendto Error " << WSAGetLastError() << endl;
        return false;
    }

    auto p = (transmission *) buffertemp;
    if (recvfrom(sClient, buffertemp, 4096, 0, (SOCKADDR *) &server, &dwSendSize) != SOCKET_ERROR) {
        //收到syn_ack
        if (p->cond == ack + syn && p->ok_seq == curr_seq && checksumchecker(buffertemp, p->seq - server_seq)) {
            cout << "收到syn_ack" << endl;
//            cout<<p->seq<<endl;
            server_seq = p->seq;
            transmission sa = transmission_message::pure_ack;
            sa.seq = curr_seq;
            server_seq++;
            sa.ok_seq = server_seq;
            sa.checksum = checksum((char *) (&sa), sizeof(transmission));
            memcpy(buffertemp, &sa, sizeof(transmission));
            nRet = sendto(sClient, buffertemp, sizeof(transmission), 0, (SOCKADDR *) &server, sizeof(SOCKADDR));
            //这可以是唯一一次发送但不改变自己的序列号的
//            curr_seq++;
            if (nRet == SOCKET_ERROR) {
                cout << "sendto Error " << WSAGetLastError() << endl;
                return false;
            }
            cout << "成功发送ack" << endl;
            return true;
        } else {
            cout << "收到非预期值" << endl;
        }
    }

    return false;
}

int client::closeConnect() {
    //用完了，关闭socket句柄(文件描述符)
    closesocket(sClient);
    WSACleanup();    //清理
    return 0;
}

int client::connectToServer() {

    if (triHandshake()) {
        cout << "三次握手成功" << endl;
    }

    return 0;
}

void client::recvBlocks(string outputdir) {
    char *buf;

    while (!shouldExit) {
        auto head = new fileBlockHead();
        //处理文件头
        buf = recvrdt(ack);
        if(shouldExit)
            break;
        auto info = (fileInfo *) buf;
        cout << info->name << " " << info->type << endl;
        head->setFilename(info->name);
        head->setType(info->type);
        head->setSize(info->size);
        head->addBlock(buf + sizeof(fileInfo), NET_BLOCK_LIMIT - sizeof(fileInfo));
        int cntBlk = 1;
        //接收余下的块
//        while ((NET_BLOCK_LIMIT * cntBlk - sizeof(fileInfo)) < head->getSize()) {
        //必须获得所有的网络块
        while(cntBlk < ceil(1.0 * (head->getSize() + sizeof(fileInfo)) / FILE_BLOCK_LIMIT) * 4){
            buf = recvrdt(ack);
            if(shouldExit)
                break;
            head->addBlock(buf, NET_BLOCK_LIMIT);
            cntBlk++;
        }

        cout<<"文件接收结束"<<endl;

//        cout << "number before: " << head->getBlkNum() << endl;
        auto toMerge = head->getHead();
        for (int i = 0; i < (int) ceil(head->getBlkNum() / 4.0); i++) {
            unifiedBlock::mergeBlock(toMerge, 4, FILE_BLOCK_LIMIT);
            toMerge = toMerge->getNext();
        }
        head->countBlocks();
//        cout << "number now: " << head->getBlkNum() << endl;

        fileLayer::putFile(outputdir, head);

        delete head;
    }

//    return head;
}

char *client::recvrdt(condition wanted, bool needNetInfo) {
    auto buffertemp = new char[4096];
    int dwSendSize = sizeof(server);
    int nRet = 0;

    //先接收一次
    auto p = (transmission *) buffertemp;
    nRet = recvfrom(sClient, buffertemp, 4096, 0, (SOCKADDR *) &server, &dwSendSize);
    if (nRet == SOCKET_ERROR) {
        cout << "sendto Error " << WSAGetLastError() << endl;
        return nullptr;
    }
    while (1) {
        if (p->cond == wanted && p->ok_seq == curr_seq && checksumchecker(buffertemp, p->seq - server_seq)) {
            //正确收到
//            pretty->pretty(static_cast<condition>(p->cond), timeGetTime(), p->seq, checksum(buffertemp, p->seq - server_seq + sizeof(transmission)));
            //开辟新的缓冲区
            auto buffer = new char[p->seq - server_seq];
            if (needNetInfo) {
                memcpy(buffer, buffertemp , p->seq - server_seq + sizeof(transmission));
            } else {
                //去掉网络头，不需要，网络头并未计入
                memcpy(buffer, buffertemp + sizeof(transmission), p->seq - server_seq);
            }
            server_seq = p->seq;
            transmission sa = transmission_message::pure_ack;
            sa.seq = curr_seq;
            //因为对面发给你的就是“这之前的”
//            server_seq++;
            sa.ok_seq = server_seq;
            sa.checksum = checksum((char *) (&sa), sizeof(transmission));
            memcpy(buffertemp, &sa, sizeof(transmission));
            nRet = sendto(sClient, buffertemp, sizeof(transmission), 0, (SOCKADDR *) &server, sizeof(SOCKADDR));
            curr_seq++;
            if (nRet == SOCKET_ERROR) {
                cout << "sendto Error " << WSAGetLastError() << endl;
                return nullptr;
            }
//            cout<<"发送正确收到ack"<<endl;
            delete buffertemp;
            return buffer;
        } else if (p->cond != fin + ack) {
            //收到非预期
            cout << "收到非预期内容" << endl;
            cout<< "recv_ok "<< p->ok_seq<< " wanted "<<curr_seq<<" cond: "<<(p->cond == ack)<<endl;

//            if(p->seq > server_seq)
//                server_seq = p->seq;

            transmission sa = transmission_message::pure_ack;
            //全都不动
            sa.seq = curr_seq;
            sa.ok_seq = server_seq;
            memset(buffertemp, 0, 4096);
            sa.checksum = checksum((char *) (&sa), sizeof(transmission));
            memcpy(buffertemp, &sa, sizeof(transmission));
            nRet = sendto(sClient, buffertemp, sizeof(transmission), 0, (SOCKADDR *) &server, sizeof(SOCKADDR));
            if (nRet == SOCKET_ERROR) {
                cout << "sendto Error " << WSAGetLastError() << endl;
                break;
            }
            cout << "发送重复ack" << endl;
            //继续等待接收
            nRet = recvfrom(sClient, buffertemp, 4096, 0, (SOCKADDR *) &server, &dwSendSize);
            if (nRet == SOCKET_ERROR) {
                cout << "sendto Error " << WSAGetLastError() << endl;
                return nullptr;
            }
        } else {
            //收到fin了，立刻退出
//            wanted = static_cast<condition>(fin + ack);
            shouldExit = true;
            cout<<"for recv fin_ack but not right "<<curr_seq<<endl;
            return nullptr;
        }
    }

    return nullptr;
}

void client::sendrdt(char *b, unsigned int size) {
    int nRet = 0;
    int dwSendSize = sizeof(server);
    auto m1 = (transmission *) b;

    //先发送一次，然后进入循环计时等待
//    cout << "send1 seq ok_seq" << m1->seq << " " << m1->ok_seq << endl;
    nRet = sendto(sClient, b, size, 0, (SOCKADDR *) &server, sizeof(SOCKADDR));
    curr_seq++;
    if (nRet == SOCKET_ERROR) {
        cout << "sendto Error " << WSAGetLastError() << endl;
        return;
    }

    DWORD timeout = 3000, t1, t2; //3s
    setsockopt(sClient, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));

    char buffertemp[4096];
    auto p = (transmission *) buffertemp;
    t1 = timeGetTime();
    nRet = recvfrom(sClient, buffertemp, 4096, 0, (SOCKADDR *) &server, &dwSendSize);
//    cout << "rcev1 seq ok_seq" << p->seq << " " << p->ok_seq << endl;

    // 等待ack
    //此时要么是收到了东西、要么是超时了
    while (1) {
        // 接收超时
        if (nRet == SOCKET_ERROR) {
            // 确实是接收超时
            if (WSAGetLastError() == 10060) {
                cout << "超时了" << endl;
                cout << "send2 seq ok_seq" << m1->seq << " " << m1->ok_seq << endl;
                nRet = sendto(sClient, b, size, 0, (SOCKADDR *) &server, sizeof(SOCKADDR));
                if (nRet == SOCKET_ERROR) {
                    cout << "sendto Error " << WSAGetLastError() << endl;
                    return;
                }
                cout << "超时返回初值" << endl;
                timeout = 3000;
                setsockopt(sClient, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
                //再次等待
                t1 = timeGetTime();
                nRet = recvfrom(sClient, buffertemp, 4096, 0, (SOCKADDR *) &server, &dwSendSize);
                cout << "rcev2 seq ok_seq" << p->seq << " " << p->ok_seq << endl;
                // 返回等待ack状态
                continue;
            } else {
                // 超时以外的未知错误
                cout << "recv Error " << WSAGetLastError() << endl;
                return;
            }
        } else {
            //正常收到
            if (p->cond == ack && p->ok_seq == curr_seq && checksumchecker(buffertemp, p->seq - server_seq)) {
                //靠的是默契而不是互相查看
//                client_seq = p->seq;
//                cout << "收到预期的ack且没有损坏，发送结束" << endl;
                pretty->pretty(static_cast<condition>(p->cond), timeGetTime(), p->seq, checksum(buffertemp, p->seq - server_seq + sizeof(transmission)));
                break;
            } else {
                cout << "损坏或不符合预期" << endl;
                //丢弃无用数据包
                memset(buffertemp, 0, 4096);
                t2 = timeGetTime();
                cout << "重置超时 " << 3000 - (double) (t2 - t1) << " ms" << endl;
                timeout = 3000 - (t2 - t1);
                setsockopt(sClient, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
                //再次等待
                t1 = timeGetTime();
                nRet = recvfrom(sClient, buffertemp, 4096, 0, (SOCKADDR *) &server, &dwSendSize);
                cout << "rcev3 seq ok_seq" << p->seq << " " << p->ok_seq << endl;
                continue;
            }
        }

    }

}

bool client::waveGoodby() {
    //四次挥手断开连接
//    char buffertemp[4096];
    char* buffer;
//    int dwSendSize = sizeof(server);
//    int nRet = 0;

//    auto m1 = transmission_message::fin_ack;
//    auto p = (transmission *) buffertemp;
//    m1.seq = curr_seq;
////    server_seq++;
//    m1.ok_seq = server_seq;
//    m1.checksum = checksum((char *) (&m1), sizeof(transmission));
//    memcpy(buffertemp, &m1, sizeof(transmission));
//    sendrdt(buffertemp, sizeof(transmission));
//    cout << "成功发送fin+ack" << endl;
//
//    if (recvfrom(sClient, buffertemp, 4096, 0, (SOCKADDR *) &server, &dwSendSize) != SOCKET_ERROR) {
//        //收到fin+ack，不是收到ack因为sendrdt里面帮你收了
//        if (p->cond == fin + ack && p->ok_seq == curr_seq && checksumchecker(buffertemp, p->seq - server_seq)) {
//            cout << "收到fin_ack" << endl;
//            server_seq = p->seq;
//            transmission sa = transmission_message::pure_ack;
//            sa.seq = curr_seq;
//            server_seq++;
//            sa.ok_seq = server_seq;
//            sa.checksum = checksum((char *) (&sa), sizeof(transmission));
//            memcpy(buffertemp, &sa, sizeof(transmission));
//            nRet = sendto(sClient, buffertemp, sizeof(transmission), 0, (SOCKADDR *) &server, sizeof(SOCKADDR));
//            curr_seq++;
//            if (nRet == SOCKET_ERROR) {
//                cout << "sendto Error " << WSAGetLastError() << endl;
//                return false;
//            }
//            cout << "成功发送ack，挥手结束" << endl;
//            return true;
//        } else {
//            cout << "收到非预期值" << endl;
//        }
//    }
//
//    return false;


    buffer = recvrdt(static_cast<condition>(fin + ack), true);
    assert(buffer != nullptr);
    cout << "成功接收到fin+ack" << endl;     //此时回复ack已经成功

    auto m1 = (transmission *) buffer;
    //发送ack+fin
    server_seq = m1->seq;
    transmission sa = transmission_message::fin_ack;
    sa.seq = curr_seq;
//    server_seq++;
    sa.ok_seq = server_seq;
    sa.checksum = checksum((char *) (&sa), sizeof(transmission));
    memcpy(buffer, &sa, sizeof(transmission));
    sendrdt(buffer, sizeof(transmission));      //会接收下一个ack
    curr_seq++;
    cout << "成功发送fin+ack，四次挥手结束" << endl;

    return true;
}

//int client::sendAndReceive() {
//    /*发送、接收消息*/
//    int ret = 0;
//    pthread_t s,r;
//    pthread_create(&s,NULL, sender,nullptr);
//    pthread_create(&r,NULL, receiver, nullptr);
//
//    pthread_join(s,nullptr);
//    pthread_join(r,nullptr);
//
//    return ret;
//}
//
//void* client::sender(void* arg) {
//
//    int ret;
//    bool noExit = true;
//    while(noExit) {
//        //取数据
//        Task t;
//        bqM2N->Get(t);
//
//        // 判断是否应该退出
//        if(t.b.m.opcode == program_exit){
//            noExit = false;
//            // 防止服务器退出
//            t.b.m.opcode = quit_group;
//            shouldExit = true;
//        }
//
//        memset(&Buffer, 0, sizeof(char)*DEFAULT_BUFFER);
//        memcpy(Buffer, &t.b.m, HEADER_LENGTH);
//        memcpy(&Buffer[HEADER_LENGTH], t.b.m.message, sizeof(char)*(strlen(t.b.m.message) + 1));
//        //向服务器发送消息
//        ret = send(sClient, Buffer, HEADER_LENGTH + strlen(t.b.m.message) + 1, 0);
//        if (ret == 0) {
//            break;
//        } else if (ret == SOCKET_ERROR) {
//            printf("send() 失败: %d\n", WSAGetLastError());
//            break;
//        }
//    }
//
//    cout<<"Client Sender Quit"<<endl;
//    return nullptr;
//}
//
//void* client::receiver(void* arg) {
//
//    int ret = 0;
//    while(!shouldExit) {
//        //接收从服务器来的消息
//        ret = recv(sClient, Buffer, DEFAULT_BUFFER, 0);
//        if (ret == 0) {
//            break;
//        } else if (ret == SOCKET_ERROR) {
//            printf("recv() 失败: %d\n", WSAGetLastError());
//            break;
//        }
//        Buffer[ret] = '\0';
//        message* m = extractHeader(&Buffer[0]);
////        printf("Recv %d bytes:\n\t%s\n", ret, m->message);
//        auto bean = new struct bean;
//        bean->socket = sClient;
//        bean->m = *m;
//        //打印接收到的消息
//        prettyPrint::pretty(*bean);
//    }
//    cout<<"Client Receiver Quit"<<endl;
//    return nullptr;
//}

