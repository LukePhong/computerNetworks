//
// Created by 'Confidence'F on 10/20/2022.
//

#include <mswsock.h>
#include <cassert>
#include "server.h"
#include "renoSender.h"
#include "gbnRenoSender.h"

bool server::shouldExit = false;

server::server(char *runAddr, unsigned short port) : port(port) {
    /*加载Winsock DLL*/
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("WinSock 初始化失败!\n");
        return;
    }

    /*创建Socket*/
    sListen = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sListen == SOCKET_ERROR) {
        printf("socket() 失败: %d\n", WSAGetLastError());
        return;
    }

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = inet_addr(runAddr);
    local.sin_port = htons(port);

    /*绑定Socket*/
    if (bind(sListen,
             (struct sockaddr *) &local,
             sizeof(local)) == SOCKET_ERROR) {
        printf("bind() 失败: %d\n", WSAGetLastError());
        return;
    }

    //使能忽略10054错误
    BOOL bEnalbeConnRestError = FALSE;
    DWORD dwBytesReturned = 0;
    WSAIoctl(sListen, SIO_UDP_CONNRESET, &bEnalbeConnRestError, sizeof(bEnalbeConnRestError),
             NULL, 0, &dwBytesReturned, NULL, NULL);

    pretty = new prettyPrint(timeGetTime());

}

int server::startListening() {
    /*打开监听*/
    return listen(sListen, 8);
}

int server::startMainLoop() {
    /*在端口进行监听，一旦有客户机发起连接请示
     就建立与客户机进行通信的线程*/
    char buffertemp[4096];
    int nRet = 0;

    while (!shouldExit) {
        /*监听是否有连接请求*/
        clientVec.emplace_back();
        sockaddr_in &client = clientVec.back();
        int dwSendSize = sizeof(client);

        cout << "等待接收第一个数据包" << endl;
        nRet = recvfrom(sListen, buffertemp, 4096, 0, (SOCKADDR *) &client, &dwSendSize);

        if (nRet == INVALID_SOCKET) {
            printf("accept() 失败: %d\n", WSAGetLastError());
            break;
        }

        if (triHandshake(buffertemp)) {
            shouldExit = 1;
            cout << "三次握手成功" << endl;
            break;
        } else {
            cout << "三次握手失败" << endl;
            break;
        }
    }

//    cout << "Server Main Loop Quit" << endl;
}

bool server::triHandshake(char *buffer) {
    char buffertemp[4096];
    sockaddr_in &client = clientVec.back();
    int dwSendSize = sizeof(client);
    int nRet = 0;

    auto m1 = (transmission *) buffer;
    if (m1->cond == syn) {
        cout << "收到syn" << endl;
//        cout<<m1->seq<<endl;
        //发送ack+syn
        client_seq = m1->seq;
        transmission sa = transmission_message::syn_ack;
        sa.seq = curr_seq;
        client_seq++;
        sa.ok_seq = client_seq;
        sa.checksum = checksum((char *) (&sa), sizeof(transmission));
        memcpy(buffertemp, &sa, sizeof(transmission));
        nRet = sendto(sListen, buffertemp, sizeof(transmission), 0, (SOCKADDR *) &client, sizeof(SOCKADDR));
        curr_seq++;
        if (nRet == SOCKET_ERROR) {
            cout << "sendto Error " << WSAGetLastError() << endl;
            return false;
        }
        cout << "成功发送syn+ack" << endl;
        //返回的是否为ack
        if (recvfrom(sListen, buffertemp, 4096, 0, (SOCKADDR *) &client, &dwSendSize)) {
            //收到ack
            m1 = (transmission *) buffertemp;
//           cout<<m1->seq<<endl;
            if (m1->cond == ack && m1->ok_seq == curr_seq && checksumchecker(buffertemp, m1->seq - client_seq)) {
                cout << "收到ack" << endl;
                return true;
            }
        }
    }

    return false;
}

int server::serverClose() {
    closesocket(sListen);
    WSACleanup();    //用完了要清理

//    for(auto& t : sendThreadVec){
//        pthread_join(t,nullptr);
//    }

    return 0;
}

void server::sendBlocks(blockHead *head) {

    long long starttime = timeGetTime();
    auto toSplit = head->getHead();
    auto toSplitNext = head->getHead()->getNext();
    for (int i = 0; i < head->getBlkNum(); ++i) {
        unifiedBlock::splitBlock(toSplit, NET_BLOCK_LIMIT);
        toSplit = toSplitNext;
        if (toSplitNext)
            toSplitNext = toSplitNext->getNext();
    }
    head->countBlocks();

    char *s;
    // the current block
    unifiedBlock *c = head->getHead();

    // int realSent = -((int) sizeof(fileInfo));
    // while (realSent < (int) (((fileBlockHead *) head)->getSize())) {
    //     struct transmission tran = transmission_message::pure_ack;
    //     //    tran.seq = curr_seq;
    //     //更新curr_seq
    //     curr_seq += c->getSize();
    //     tran.seq = curr_seq;
    //     tran.ok_seq = client_seq++;

    //     s = new char[sizeof(transmission) + c->getSize()];
    //     memcpy(s, &tran, sizeof(transmission));
    //     memcpy(s + sizeof(transmission), c->getBuffer(), c->getSize());
    //     ((transmission *) s)->checksum = checksum(s, sizeof(transmission) + c->getSize());

    //     // rdt发送
    //     sendrdt(s, sizeof(transmission) + c->getSize());

    //     c = c->getNext();
    //     realSent += c->getSize();
    //     delete[]s;
    // }

//    send_gbn(c);
//    send_gbn_timer(c);

    auto r = new renoSender(this);
//   auto r = new gbnRenoSender(this);
   r->renoSend(c);

    pretty->outputSpeed(timeGetTime(), starttime, ((fileBlockHead*)head)->getSize());
}

void server::send_gbn(const unifiedBlock *c){

    sockaddr_in &client = clientVec.back();
    int dwSendSize = sizeof(client);

    bool needResend = false;
    queue<char*> windowQueue;
    queue<long long> timeQueue;
    queue<const unifiedBlock*> wait4ack;
    // 窗口大小、base和next都使用字节数，这是比较好适应网络发送需求也体现了解耦
    size_t N = 16384;
    size_t base = curr_seq, next = curr_seq;
    char buffertemp[4096];
    //超时：3s
    long long timeout = 3000;

    // 第一次批量发送要使用的client_seq
//    client_seq++;
//    unsigned int cseq4check = client_seq;

    while(1){

        if(needResend){
            queue<long long>().swap(timeQueue);
            size_t queueLength = windowQueue.size();
            assert (windowQueue.size() == wait4ack.size());
            cout<<"进行超时重传"<<endl;
            for (int i = 0; i < queueLength; i++) {
                sendto(sListen, windowQueue.front(), sizeof(transmission) + wait4ack.front()->getSize(), 0, (SOCKADDR *) &client, sizeof(SOCKADDR));
                timeQueue.push(timeGetTime());
                windowQueue.push(windowQueue.front());
                windowQueue.pop();
                wait4ack.push(wait4ack.front());
                wait4ack.pop();
            }
            needResend = false;
        }else if(c){
            struct transmission tran = transmission_message::pure_ack;
            //更新curr_seq
            curr_seq += c->getSize();
            tran.seq = curr_seq;
            tran.ok_seq = client_seq++;

            auto s = new char[sizeof(transmission) + c->getSize()];
            memcpy(s, &tran, sizeof(transmission));
            memcpy(s + sizeof(transmission), c->getBuffer(), c->getSize());
            ((transmission *) s)->checksum = checksum(s, sizeof(transmission) + c->getSize());

            windowQueue.push(s);
//            windowQueue.pop();

            sendto(sListen, s, sizeof(transmission) + c->getSize(), 0, (SOCKADDR *) &client, sizeof(SOCKADDR));

//            cout<<"正常发送 大小： "<<c->getSize()<<endl;

//            if(base == next){
                timeQueue.push(timeGetTime());
//            }
            wait4ack.push(c);
            next += c->getSize();
            c = c->getNext();
            if(c && next < base + N)
                continue;
        }

        size_t t, diff;
        transmission *p;
        while(1) {
            recvfrom(sListen, buffertemp, 4096, 0, (SOCKADDR *) &client, &dwSendSize);
            p = (transmission*)buffertemp;
            t = p->ok_seq;
            diff = t - base;
            //防止最后出现丢包
            if (p->cond == ack && (diff > 0 || c == nullptr) && checksumchecker(buffertemp, 0)){
//                client_seq = p->seq + 1;
                break;
            }
        }
        base = t;
//        cout<<"更新base diff: "<<diff<<endl;
//        pretty->outputBaseNext(static_cast<condition>(p->cond), timeGetTime(), p->seq, checksum(buffertemp, sizeof(transmission)), base, next);
        size_t acked = 0;
        if(diff > 0){
            while(acked < diff){
                acked += wait4ack.front()->getSize();
                wait4ack.pop();
                timeQueue.pop();
                assert(windowQueue.front() != nullptr);
                delete[] windowQueue.front();
                windowQueue.pop();
            }
        }

        if(timeGetTime() - timeQueue.front() > timeout){
            needResend = true;
            cout<<"发现超时"<<endl;
        }

        if (c == nullptr && wait4ack.empty()){
            assert(windowQueue.empty());
            assert(timeQueue.empty());
            break;
        }
    }

    cout<<"发送结束"<<endl;
}

bool needRe;
void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    needRe = true;
}

void server::send_gbn_timer(const unifiedBlock *c){

    needRe = false;
    bool isTimeout = false;

    sockaddr_in &client = clientVec.back();
    int dwSendSize = sizeof(client);

    queue<char*> windowQueue;
//    queue<long long> timeQueue;
    queue<const unifiedBlock*> wait4ack;
    // 窗口大小、base和next都使用字节数，这是比较好适应网络发送需求也体现了解耦
    size_t N = 32768;
    size_t base = curr_seq, next = curr_seq;
    char buffertemp[4096];
    //超时：3s
    long long timeout = 3000;


    while(1){

        if(needRe){
            size_t queueLength = windowQueue.size();
            assert (windowQueue.size() == wait4ack.size());
            cout<<"进行超时重传"<<endl;
            SetTimer(NULL, 1, timeout, (TIMERPROC)TimerProc);
            for (int i = 0; i < queueLength; i++) {
                sendto(sListen, windowQueue.front(), sizeof(transmission) + wait4ack.front()->getSize(), 0, (SOCKADDR *) &client, sizeof(SOCKADDR));
                windowQueue.push(windowQueue.front());
                windowQueue.pop();
                wait4ack.push(wait4ack.front());
                wait4ack.pop();
            }
            needRe = false;
            isTimeout = true;
        }else if(c && !isTimeout){
            struct transmission tran = transmission_message::pure_ack;
            //更新curr_seq
            curr_seq += c->getSize();
            tran.seq = curr_seq;
            tran.ok_seq = client_seq++;

            auto s = new char[sizeof(transmission) + c->getSize()];
            memcpy(s, &tran, sizeof(transmission));
            memcpy(s + sizeof(transmission), c->getBuffer(), c->getSize());
            ((transmission *) s)->checksum = checksum(s, sizeof(transmission) + c->getSize());

            windowQueue.push(s);

            sendto(sListen, s, sizeof(transmission) + c->getSize(), 0, (SOCKADDR *) &client, sizeof(SOCKADDR));

            if(base == next){
                SetTimer(NULL, 1, timeout, (TIMERPROC)TimerProc);
            }

            wait4ack.push(c);
            next += c->getSize();
            c = c->getNext();
            if(c && next < base + N)
                continue;
        }

        size_t t, diff;
        transmission *p;
        while(1) {
            recvfrom(sListen, buffertemp, 4096, 0, (SOCKADDR *) &client, &dwSendSize);
            p = (transmission*)buffertemp;
            t = p->ok_seq;
            diff = t - base;
            //防止最后出现丢包
            if (p->cond == ack && (diff > 0 || c == nullptr) && checksumchecker(buffertemp, 0)){
                break;
            }
        }
        base = t;
        pretty->outputBaseNext(static_cast<condition>(p->cond), timeGetTime(), p->seq, checksum(buffertemp, sizeof(transmission)), base, next);
        size_t acked = 0;
        if(diff > 0){
            while(acked < diff){
                acked += wait4ack.front()->getSize();
                wait4ack.pop();
//                timeQueue.pop();
                assert(windowQueue.front() != nullptr);
                delete[] windowQueue.front();
                windowQueue.pop();
            }
        }

        if(base == next){
            KillTimer(NULL, 1);
            //重传完成
            isTimeout = false;
        }else{
            //reset timer
            KillTimer(NULL, 1);
            SetTimer(NULL, 1, timeout, (TIMERPROC)TimerProc);
        }

        if (c == nullptr && wait4ack.empty()){
            assert(windowQueue.empty());
//            assert(timeQueue.empty());
            break;
        }
    }

    cout<<"发送结束"<<endl;
}

void server::sendrdt(char *b, unsigned int size) {
    int nRet = 0;
    sockaddr_in &client = clientVec.back();
    int dwSendSize = sizeof(client);
    auto m1 = (transmission *) b;

    //先发送一次，然后进入循环计时等待
//    cout << "send1 seq ok_seq" << m1->seq << " " << m1->ok_seq << endl;
    nRet = sendto(sListen, b, size, 0, (SOCKADDR *) &client, sizeof(SOCKADDR));
    //因为对面不会再加了
//    curr_seq++;
    if (nRet == SOCKET_ERROR) {
        cout << "sendto Error " << WSAGetLastError() << endl;
        return;
    }

    DWORD timeout = 3000, t1, t2; //3s
    setsockopt(sListen, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
//    unsigned long ul=1;
//    ioctlsocket(sListen, FIONBIO, (unsigned long *)&ul);//设置成非阻塞模式。

    char buffertemp[4096];
    auto p = (transmission *) buffertemp;
    t1 = timeGetTime();
    nRet = recvfrom(sListen, buffertemp, 4096, 0, (SOCKADDR *) &client, &dwSendSize);
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
                nRet = sendto(sListen, b, size, 0, (SOCKADDR *) &client, sizeof(SOCKADDR));
                if (nRet == SOCKET_ERROR) {
                    cout << "sendto Error " << WSAGetLastError() << endl;
                    return;
                }
                cout << "超时返回初值" << endl;
                timeout = 3000;
                setsockopt(sListen, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
                //再次等待
                t1 = timeGetTime();
                nRet = recvfrom(sListen, buffertemp, 4096, 0, (SOCKADDR *) &client, &dwSendSize);
                cout << "rcev2 seq ok_seq" << p->seq << " " << p->ok_seq << endl;
                // 返回等待ack状态
                continue;
            }
            else {
                // 超时以外的未知错误
                cout << "recv Error " << WSAGetLastError() << endl;
                return;
            }
        } else {
            //正常收到
            if (p->cond == ack && p->ok_seq == curr_seq && checksumchecker(buffertemp, 0)) {
//                client_seq = p->seq;
//                cout << "收到预期的ack且没有损坏，发送结束" << endl;
                break;
            } else {
                cout << "损坏或不符合预期" << endl;
                //丢弃无用数据包
                memset(buffertemp, 0, 4096);
                t2 = timeGetTime();
                cout << "重置超时 " << 3000 - (double) (t2 - t1) << " ms" << endl;
                timeout = 3000 - (t2 - t1);
                setsockopt(sListen, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
                //再次等待
                t1 = timeGetTime();
                nRet = recvfrom(sListen, buffertemp, 4096, 0, (SOCKADDR *) &client, &dwSendSize);
                cout << "rcev3 seq ok_seq" << p->seq << " " << p->ok_seq << endl;
                continue;
            }
        }

    }

}

char *server::recvrdt(condition wanted, bool needNetInfo) {
    auto buffertemp = new char[4096];
    sockaddr_in &client = clientVec.back();
    int dwSendSize = sizeof(client);
    int nRet = 0;

    //先接收一次
    auto p = (transmission *) buffertemp;
    nRet = recvfrom(sListen, buffertemp, 4096, 0, (SOCKADDR *) &client, &dwSendSize);
    if (nRet == SOCKET_ERROR) {
        cout << "sendto Error1 " << WSAGetLastError() << endl;
        return nullptr;
    }
    while (1) {
        if (p->cond == wanted && p->ok_seq == curr_seq && checksumchecker(buffertemp, p->seq - client_seq)) {
            //正确收到
//            cout<<"已正确收到"<<endl;
            //开辟新的缓冲区
            auto buffer = new char[p->seq - client_seq];
            if (needNetInfo) {
                memcpy(buffer, buffertemp , p->seq - client_seq + sizeof(transmission));
            } else {
                //去掉网络头，不需要，网络头并未计入
                memcpy(buffer, buffertemp + sizeof(transmission), p->seq - client_seq);
            }
            client_seq = p->seq;
            transmission sa = transmission_message::pure_ack;
            sa.seq = curr_seq;
            client_seq++;
            sa.ok_seq = client_seq;
            sa.checksum = checksum((char *) (&sa), sizeof(transmission));
            memcpy(buffertemp, &sa, sizeof(transmission));
            nRet = sendto(sListen, buffertemp, sizeof(transmission), 0, (SOCKADDR *) &client, sizeof(SOCKADDR));
            curr_seq++;
            if (nRet == SOCKET_ERROR) {
                cout << "sendto Error2 " << WSAGetLastError() << endl;
                return nullptr;
            }
//            cout<<"发送正确收到ack"<<endl;
            delete buffertemp;
            return buffer;
        } else {
            //收到非预期
//            cout << "收到非预期内容" << endl;
            transmission sa = transmission_message::pure_ack;
            //全都不动
            sa.seq = curr_seq;
            sa.ok_seq = client_seq;
            memset(buffertemp, 0, 4096);
            sa.checksum = checksum((char *) (&sa), sizeof(transmission));
            memcpy(buffertemp, &sa, sizeof(transmission));
            nRet = sendto(sListen, buffertemp, sizeof(transmission), 0, (SOCKADDR *) &client, sizeof(SOCKADDR));
            if (nRet == SOCKET_ERROR) {
                cout << "sendto Error3 " << WSAGetLastError() << endl;
                break;
            }
//            cout << "发送重复ack" << endl;
            //继续等待接收
            nRet = recvfrom(sListen, buffertemp, 4096, 0, (SOCKADDR *) &client, &dwSendSize);
            if (nRet == SOCKET_ERROR) {
                cout << "sendto Error4 " << WSAGetLastError() << endl;
                return nullptr;
            }
        }
    }

    return nullptr;
}

bool server::waveGoodby() {
//    char *buffer;
    char buffertemp[4096];
    sockaddr_in &client = clientVec.back();
    int dwSendSize = sizeof(client);
    int nRet = 0;

    auto m1 = transmission_message::fin_ack;
    auto p = (transmission *) buffertemp;
    m1.seq = curr_seq;
//    client_seq++;
    m1.ok_seq = client_seq;
    m1.checksum = checksum((char *) (&m1), sizeof(transmission));
    memcpy(buffertemp, &m1, sizeof(transmission));
    sendrdt(buffertemp, sizeof(transmission));
    cout << "成功发送fin+ack" << endl;

    if (recvfrom(sListen, buffertemp, 4096, 0, (SOCKADDR *) &client, &dwSendSize) != SOCKET_ERROR) {
        //收到fin+ack，不是收到ack因为sendrdt里面帮你收了
        if (p->cond == fin + ack && p->ok_seq == curr_seq && checksumchecker(buffertemp, p->seq - client_seq)) {
            cout << "收到fin_ack" << endl;
            client_seq = p->seq;
            transmission sa = transmission_message::pure_ack;
            sa.seq = curr_seq;
            client_seq++;
            sa.ok_seq = client_seq;
            sa.checksum = checksum((char *) (&sa), sizeof(transmission));
            memcpy(buffertemp, &sa, sizeof(transmission));
            nRet = sendto(sListen, buffertemp, sizeof(transmission), 0, (SOCKADDR *) &client, sizeof(SOCKADDR));
            curr_seq++;
            if (nRet == SOCKET_ERROR) {
                cout << "sendto Error " << WSAGetLastError() << endl;
                return false;
            }
            cout << "成功发送ack，挥手结束" << endl;
            return true;
        } else {
            cout << "收到非预期值" << endl;
        }
    }

    return false;
}

unsigned int server::getCurrSeq() const {
    return curr_seq;
}

unsigned int server::getClientSeq() const {
    return client_seq;
}

bool server::sendPacket(char *buf,int len) {
    if(!buf){
        pretty->error("sendPacket got nullptr");
        return true;
    }

//    auto t = (transmission*)buf;
//    if(t->ok_seq < client_seq) {
//        t->ok_seq = client_seq;
//        t->checksum = 0;
//        t->checksum = checksum(buf, len);
//    }

    int nRet = 0;
    nRet = sendto(sListen, buf, len, 0, (SOCKADDR *) &clientVec.back(), sizeof(SOCKADDR));
    if (nRet == SOCKET_ERROR) {
        pretty->error("sendto error " + to_string(WSAGetLastError()));
        return true;
    }
    return false;
}

bool server::receivePacket(char *buf,int len) {

    int nRet = 0;
    int dwSendSize = sizeof(clientVec.back());
    nRet = recvfrom(sListen, buf, len, 0, (SOCKADDR *) &clientVec.back(), &dwSendSize);
    if (nRet == SOCKET_ERROR) {
        pretty->error("sendto error " + to_string(WSAGetLastError()));
        return true;
    }
    auto t = (transmission*)buf;
    if(t->seq > client_seq)
        client_seq = t->seq - 1;

    return false;
}

prettyPrint *server::getPretty() const {
    return pretty;
}

void server::updateCurrSeq(unsigned int length) {
    curr_seq += length;
}

void server::updateClientSeq() {
    client_seq++;
}





//
//void *server::sendToClient(void *arg) {
//
//    channel* c = (channel*)arg;
//    BlockQueue* myQueue = c->in;
//    SOCKET mySocket = c->out;
//    char Buffer[DEFAULT_BUFFER];
//    cout<<"send thread create finished"<<endl;
//    bool noExit = true;
//    while(noExit){
//        //取数据
//        Task t;
//        myQueue->Get(t);
//
//        // 判断是否应该退出
//        if(t.b.m.opcode == program_exit){
//            noExit = false;
//            shouldExit = true;
//        }
//        // 拷贝首部和消息本体到Buffer
//        memset(&Buffer, 0, sizeof(char)*DEFAULT_BUFFER);
//        memcpy(Buffer, &t.b.m, HEADER_LENGTH);
//        memcpy(&Buffer[HEADER_LENGTH], t.b.m.message, sizeof(char)*(strlen(t.b.m.message) + 1));
//        int ret = send(mySocket, Buffer, HEADER_LENGTH + strlen(t.b.m.message) + 1, 0);
//        if (ret == 0) {
//            break;
//        } else if (ret == SOCKET_ERROR) {
//            printf("send() 失败: %d\n", WSAGetLastError());
//            break;
//        }
//        printf("Server Send %d bytes\n", ret);
//    }
//
//    cout<<"Server Sender Quit"<<endl;
//    return nullptr;
//}
//
//message *server::extractHeader(char *buffer) {
//
//    char* heapBuffer = new char[DEFAULT_BUFFER];
//    memcpy(heapBuffer, buffer, sizeof(char) * DEFAULT_BUFFER);
//    message* m = (message*)heapBuffer;
//    m->message = &heapBuffer[HEADER_LENGTH];
//
//    return m;
//}

//从client接收消息
//DWORD server::ClientThread(LPVOID lpParam) {
//    SOCKET sock = (SOCKET) lpParam;
//    // 每个线程有一个
//    char Buffer[DEFAULT_BUFFER];
//    int ret;
//    while (!shouldExit) {
//        /*接收来自客户机的消息*/
//        // 清空Buffer
//        memset(&Buffer, 0, sizeof(char)*DEFAULT_BUFFER);
//        //接收消息
//        ret = recv(sock, Buffer, DEFAULT_BUFFER, 0);
//        if (ret == 0)
//            break;
//        else if (ret == SOCKET_ERROR) {
//            printf("recv() 失败: %d\n", WSAGetLastError());
//            break;
//        }
//        Buffer[ret] = '\0';
//        //消息长度
//        cout<<"ret: "<<ret<<endl;
//        //拆出首部
//        message* m = extractHeader(&Buffer[0]);
//        //第一层封装
//        auto bean = new struct bean;
//        bean->socket = sock;
//        bean->m = *m;
//        //打印接收到的消息
//        prettyPrint::pretty(*bean);
//        //第二层封装
//        Task t(*bean);
//        pthread_mutex_lock(&toMainLock);
//        bqN2M->Put(t);
//        pthread_mutex_unlock(&toMainLock);
//    }
//
//    cout<<"Server Receiver Quit"<<endl;
//    return 0;
//}
//








