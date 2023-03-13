//
// Created by 'Confidence'F on 10/20/2022.
//

#include "client.h"

char client::Buffer[];
SOCKET client::sClient;
BlockQueue* client::bqM2N;

bool client::shouldExit = false;

client::client(char* hostAddress, unsigned short port) : port(port) {
    /*加载Winsock DLL*/
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("Winsock    初始化失败!\n");
        return;
    }

    /*创建Socket*/
    sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
}

int client::connectToServer() {
    /*与服务器建立连接*/
    if (connect(sClient, (struct sockaddr *) &server,
                sizeof(server)) == SOCKET_ERROR) {
        printf("connect() 失败: %d\n", WSAGetLastError());
        return 1;
    }
}

int client::closeConnect() {
    //用完了，关闭socket句柄(文件描述符)
    closesocket(sClient);
    WSACleanup();    //清理
    return 0;
}

int client::sendAndReceive() {
    /*发送、接收消息*/
    int ret = 0;
    pthread_t s,r;
    pthread_create(&s,NULL, sender,nullptr);
    pthread_create(&r,NULL, receiver, nullptr);

    pthread_join(s,nullptr);
    pthread_join(r,nullptr);

    return ret;
}

void* client::sender(void* arg) {

    int ret;
    bool noExit = true;
    while(noExit) {
        //取数据
        Task t;
        bqM2N->Get(t);

        // 判断是否应该退出
        if(t.b.m.opcode == program_exit){
            noExit = false;
            // 防止服务器退出
            t.b.m.opcode = quit_group;
            shouldExit = true;
        }

        memset(&Buffer, 0, sizeof(char)*DEFAULT_BUFFER);
        memcpy(Buffer, &t.b.m, HEADER_LENGTH);
        memcpy(&Buffer[HEADER_LENGTH], t.b.m.message, sizeof(char)*(strlen(t.b.m.message) + 1));
        //向服务器发送消息
        ret = send(sClient, Buffer, HEADER_LENGTH + strlen(t.b.m.message) + 1, 0);
        if (ret == 0) {
            break;
        } else if (ret == SOCKET_ERROR) {
            printf("send() 失败: %d\n", WSAGetLastError());
            break;
        }
    }

    cout<<"Client Sender Quit"<<endl;
    return nullptr;
}

void* client::receiver(void* arg) {

    int ret = 0;
    while(!shouldExit) {
        //接收从服务器来的消息
        ret = recv(sClient, Buffer, DEFAULT_BUFFER, 0);
        if (ret == 0) {
            break;
        } else if (ret == SOCKET_ERROR) {
            printf("recv() 失败: %d\n", WSAGetLastError());
            break;
        }
        Buffer[ret] = '\0';
        message* m = extractHeader(&Buffer[0]);
//        printf("Recv %d bytes:\n\t%s\n", ret, m->message);
        auto bean = new struct bean;
        bean->socket = sClient;
        bean->m = *m;
        //打印接收到的消息
        prettyPrint::pretty(*bean);
    }
    cout<<"Client Receiver Quit"<<endl;
    return nullptr;
}

//产生一个消息头
message* client::formMessage(char *Buffer) {

    char* heapStr = new char[strlen(Buffer) + 1];
    strcpy(heapStr, Buffer);
    message* message = new struct message;
    message->opcode = 0;
    strcpy(message->user_name, "default");
    message->time = time(nullptr);
    message->length = strlen(Buffer);
    message->message = heapStr;

    return message;
}

message *client::extractHeader(char *buffer) {

    char* heapBuffer = new char[DEFAULT_BUFFER];
    memcpy(heapBuffer, buffer, sizeof(char) * DEFAULT_BUFFER);
    message* m = (message*)heapBuffer;
    m->message = &heapBuffer[HEADER_LENGTH];

    return m;
}