//
// Created by 'Confidence'F on 10/20/2022.
//

#include "server.h"

pthread_mutex_t server::bqM2NLock;

bool server::shouldExit = false;

server::server(unsigned short port, map<SOCKET, BlockQueue*>* bqM2N): port(port), bqM2N(bqM2N) {
    /*加载Winsock DLL*/
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("WinSock 初始化失败!\n");
        return;
    }

    /*创建Socket*/
    sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sListen == SOCKET_ERROR) {
        printf("socket() 失败: %d\n", WSAGetLastError());
        return;
    }

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);

    /*绑定Socket*/
    if (bind(sListen,
             (struct sockaddr *) &local,
             sizeof(local)) == SOCKET_ERROR) {
        printf("bind() 失败: %d\n", WSAGetLastError());
        return;
    }

    // 初始化和主进程通信的锁
    pthread_mutex_init(&toMainLock, NULL);
    // 初始化主进程发回信道的锁
    pthread_mutex_init(&bqM2NLock, NULL);
}

int server::startListening() {
    /*打开监听*/
    return listen(sListen, 8);
}

int server::startMainLoop() {
    /*在端口进行监听，一旦有客户机发起连接请示
     就建立与客户机进行通信的线程*/
    while (!shouldExit) {
        /*监听是否有连接请求*/
        clientVec.emplace_back();
        sockaddr_in& client = clientVec.back();
        // 创建新的sClient
        sClient = accept(sListen, (struct sockaddr*)&client, &AddrSize);
        if (sClient == INVALID_SOCKET) {
            printf("accept() 失败: %d\n", WSAGetLastError());
            break;
        }
        printf("接受客户端连接: %s:%d\n",
               inet_ntoa(client.sin_addr),
               ntohs(client.sin_port));

        dwThreadVec.emplace_back();
        //创建一个接收线程
        hThreadVec.push_back(CreateThread(NULL, 0, ClientThread,
                               (LPVOID)sClient, 0, &dwThreadVec.back()));
        //创建一个发送线程
        sendThreadVec.emplace_back();
        // 每个从main接收的信道都是一个生产、一个发送，不需要在生产发送过程中抢锁
        pthread_t& newSend = sendThreadVec.back();
        // 避免同时操作vector
        pthread_mutex_lock(&bqM2NLock);
        (*bqM2N)[sClient] = new BlockQueue(5);
        pthread_mutex_unlock(&bqM2NLock);
        channel c{(*bqM2N)[sClient], sClient};
        pthread_create(&newSend, NULL, sendToClient, (void*)&c);

        HANDLE& hThread = hThreadVec.back();
        if (hThread == NULL) {
            printf("CreateThread() 失败: %d\n", GetLastError());
            break;
        }
        //处理完后关闭
        //回收句柄而不是关闭线程
        CloseHandle(hThread);
    }

    cout<<"Server Main Loop Quit"<<endl;
}

//从client接收消息
DWORD server::ClientThread(LPVOID lpParam) {
    SOCKET sock = (SOCKET) lpParam;
    // 每个线程有一个
    char Buffer[DEFAULT_BUFFER];
    int ret;
    while (!shouldExit) {
        /*接收来自客户机的消息*/
        // 清空Buffer
        memset(&Buffer, 0, sizeof(char)*DEFAULT_BUFFER);
        //接收消息
        ret = recv(sock, Buffer, DEFAULT_BUFFER, 0);
        if (ret == 0)
            break;
        else if (ret == SOCKET_ERROR) {
            printf("recv() 失败: %d\n", WSAGetLastError());
            break;
        }
        Buffer[ret] = '\0';
        //消息长度
        cout<<"ret: "<<ret<<endl;
        //拆出首部
        message* m = extractHeader(&Buffer[0]);
        //第一层封装
        auto bean = new struct bean;
        bean->socket = sock;
        bean->m = *m;
        //打印接收到的消息
        prettyPrint::pretty(*bean);
        //第二层封装
        Task t(*bean);
        pthread_mutex_lock(&toMainLock);
        bqN2M->Put(t);
        pthread_mutex_unlock(&toMainLock);
    }

    cout<<"Server Receiver Quit"<<endl;
    return 0;
}

int server::serverClose() {
    closesocket(sListen);
    WSACleanup();    //用完了要清理

    for(auto& t : sendThreadVec){
        pthread_join(t,nullptr);
    }

    return 0;
}

void *server::sendToClient(void *arg) {

    channel* c = (channel*)arg;
    BlockQueue* myQueue = c->in;
    SOCKET mySocket = c->out;
    char Buffer[DEFAULT_BUFFER];
    cout<<"send thread create finished"<<endl;
    bool noExit = true;
    while(noExit){
        //取数据
        Task t;
        myQueue->Get(t);

        // 判断是否应该退出
        if(t.b.m.opcode == program_exit){
            noExit = false;
            shouldExit = true;
        }
        // 拷贝首部和消息本体到Buffer
        memset(&Buffer, 0, sizeof(char)*DEFAULT_BUFFER);
        memcpy(Buffer, &t.b.m, HEADER_LENGTH);
        memcpy(&Buffer[HEADER_LENGTH], t.b.m.message, sizeof(char)*(strlen(t.b.m.message) + 1));
        int ret = send(mySocket, Buffer, HEADER_LENGTH + strlen(t.b.m.message) + 1, 0);
        if (ret == 0) {
            break;
        } else if (ret == SOCKET_ERROR) {
            printf("send() 失败: %d\n", WSAGetLastError());
            break;
        }
        printf("Server Send %d bytes\n", ret);
    }

    cout<<"Server Sender Quit"<<endl;
    return nullptr;
}

message *server::extractHeader(char *buffer) {

    char* heapBuffer = new char[DEFAULT_BUFFER];
    memcpy(heapBuffer, buffer, sizeof(char) * DEFAULT_BUFFER);
    message* m = (message*)heapBuffer;
    m->message = &heapBuffer[HEADER_LENGTH];

    return m;
}
