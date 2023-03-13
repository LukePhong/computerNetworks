//
// Created by 'Confidence'F on 10/20/2022.
//
#include <sstream>
#include <map>
#include <string>
#include <set>
using namespace std;

#include "server.h"
#include "client.h"

char* hostAddr = "127.0.0.1";
//vector<BlockQueue*>* bqM2N = new vector<BlockQueue*>;
map<SOCKET, BlockQueue*>* bqM2N = new map<SOCKET, BlockQueue*>;
BlockQueue* bqN2M = new BlockQueue(5);
BlockQueue* server::bqN2M;
pthread_mutex_t toMain;
pthread_mutex_t& server::toMainLock = toMain;

/* variables for client */
char user_name[128];
int groupId = -1;
/* variables for server */
//查找：currGroup->group->socket_username
map<SOCKET, char*> socket_username;
map<int, set<SOCKET>> groups;
map<SOCKET, int> currGroup;

/*函数声明*/
void *networks(void *arg);
void* clientController(void* arg);
void* serverController(void* arg);
void* inputCollector(void* arg);
bean getGroupInfo();
message *formMessage(char *Buffer);
opcode stringToOp(string s);

int main(int argc, char **argv) {
    //中文输出
    system("chcp 65001");

    int mode = atoi(argv[1]);
    pthread_t n, c, i;
    if(mode) {
        pthread_create(&c, NULL, clientController, nullptr);
    }else{
        pthread_create(&c, NULL, serverController, nullptr);
    }
    pthread_create(&i, NULL, inputCollector, nullptr);
    pthread_create(&n,NULL, networks,(void*)&mode);

    pthread_join(i,nullptr);
    pthread_join(n,nullptr);
    pthread_join(c, nullptr);

    return 0;
}


void *networks(void *arg) {

    int* mode = (int*)arg;
    // 1=客户端 0=服务器
    if (*mode == 1){
        // client默认这里是0
        (*bqM2N)[0] = new BlockQueue(5);
        client::bqM2N = (*bqM2N)[0];
        // 指定服务器地址和端口号
        client clt(hostAddr, 9999);
        clt.connectToServer();
        clt.sendAndReceive();
        clt.closeConnect();
    }else{
        // 服务器端口号和控制-网络队列
        server srv(9999, bqM2N);
        server::bqN2M = bqN2M;
        strcpy(user_name, "Server");
        srv.startListening();
        srv.startMainLoop();
        srv.serverClose();
    }

    return nullptr;
}

void* clientController(void* arg){
    Task t;

    bool noExit = true;
    while(noExit)
    {
        if(strlen(user_name) == 0){
            cout<<"Please enter user name"<<endl;
            bqN2M->Get(t);
            if(t.b.m.opcode != user_registration){
                cout<<"Please enter user name. Use :c name"<<endl;
                continue;
            }
            strcpy(user_name, t.b.m.message);
            pthread_mutex_lock(&server::bqM2NLock);
            (*bqM2N)[0]->Put(t);
            pthread_mutex_unlock(&server::bqM2NLock);
            continue;
        }
        if(groupId == -1){
            cout<<"Please choose a group id"<<endl;
            bqN2M->Get(t);
            if(t.b.m.opcode != join){
                cout<<"Please choose a group id. Use :c join"<<endl;
                continue;
            }
            groupId = atoi(t.b.m.message);
            pthread_mutex_lock(&server::bqM2NLock);
            (*bqM2N)[0]->Put(t);
            pthread_mutex_unlock(&server::bqM2NLock);
            continue;
        }

        bqN2M->Get(t);
        switch(t.b.m.opcode){
            case user_registration:
                strcpy(user_name, t.b.m.message);
                break;
            case user_group_info:
                break;
            case plan_text:
                break;
            case quit_group:
                groupId = -1;
                break;
            case program_exit:
                noExit = false;
                break;
            default:
                cout<<"Unsupported opcode "<<t.b.m.opcode<<" Omitted"<<endl;
                continue;
        }
        strcpy(t.b.m.user_name, user_name);

        // 向每一个通道发数据
        pthread_mutex_lock(&server::bqM2NLock);
        (*bqM2N)[0]->Put(t);
        pthread_mutex_unlock(&server::bqM2NLock);
    }
}

void* serverController(void* arg){
    Task t, newT;
    bool noExit = true;
    while(noExit){
        //从N2M队列取数据
        bqN2M->Get(t);

        switch(t.b.m.opcode){
            case user_registration:
                //用户名注册
                //两个map要同步
                if(socket_username.count(t.b.socket) == 0)
                    groups[groups.size()].insert(t.b.socket);
                socket_username[t.b.socket] = t.b.m.message;
                cout<<"registered: "<<t.b.m.message<<endl;
                // 发送可用群组信息表（写阻塞队列）
                newT.b = getGroupInfo();
                pthread_mutex_lock(&server::bqM2NLock);
                (*bqM2N)[t.b.socket]->Put(newT);
                pthread_mutex_unlock(&server::bqM2NLock);
                continue;

            case join:
                //用户加入，修改对应的数据结构
                if(socket_username.count(t.b.socket)) {
                    groups[atoi(t.b.m.message)].insert(t.b.socket);
                    currGroup[t.b.socket] = atoi(t.b.m.message);
                    cout<<"successfully joined"<<endl;
                }
                continue;

            case quit_group:
                //用户退出
                groups[currGroup[t.b.socket]].erase(t.b.socket);
                // 发送可用群组信息表（写阻塞队列）
                newT.b = getGroupInfo();
                pthread_mutex_lock(&server::bqM2NLock);
                (*bqM2N)[t.b.socket]->Put(newT);
                pthread_mutex_unlock(&server::bqM2NLock);
            case plan_text:
                //普通文字信息
                //根据保存的群组信息，将消息装入对应的阻塞队列中
                for (auto i:groups[currGroup[t.b.socket]]) {
                    pthread_mutex_lock(&server::bqM2NLock);
                    (*bqM2N)[i]->Put(t);
                    pthread_mutex_unlock(&server::bqM2NLock);
                }
                continue;
            case program_exit:
                // 服务器退出
                noExit = false;
                // 通知每一个线程
                pthread_mutex_lock(&server::bqM2NLock);
                for(auto& bqPtr : *bqM2N){
                    bqPtr.second->Put(t);
                }
                pthread_mutex_unlock(&server::bqM2NLock);
                break;
            default:
                break;
        }
    }
    cout<<"Server Controller Quit"<<endl;
}


void* inputCollector(void* arg){
    char Buffer[4096];
//    char* Buffer;
    bool noExit = true;
    while(noExit) {
//        Buffer = new char[4096];
        gets(Buffer);
        // 封装成message
        message *m = formMessage(Buffer);
        // 封装成bean
        auto bean = new struct bean;
        bean->m = *m;

        stringstream ss(Buffer);
        string temp;
        ss>>temp;
        char tempName[128];
        if(temp == ":c"){
            ss>>temp;
            opcode op = stringToOp(temp);
            bean->m.opcode = op;
            bean->m.time = time(NULL);
            switch(op){
                case user_registration:
                case join:
                    ss>>temp;
                    strcpy(bean->m.message, temp.c_str());
                    break;
                case quit_group:
                    strcpy(tempName, user_name);
                    strcpy(bean->m.message, strcat(tempName, " quits"));
                    break;
                case program_exit:
                    strcpy(tempName, user_name);
                    strcpy(bean->m.message, strcat(tempName, " exits"));
                    noExit = false;
                    break;
                default:
                    cout<<"Unsupported opcode "<<op<<" Omitted"<<endl;
                    continue;
            }
        }else {
            bean->m.opcode = plan_text;
        }
        // 送入queue
        Task t(*bean);
        pthread_mutex_lock(&toMain);
        bqN2M->Put(t);
        pthread_mutex_unlock(&toMain);
    }

    cout<<"Input Controller Quit"<<endl;
}

message *formMessage(char *Buffer) {

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

bean getGroupInfo(){
    bean b;
    b.m.opcode = user_group_info;
    strcpy(b.m.user_name, "Server");
    b.m.time = time(NULL);
    auto Buffer = new char[DEFAULT_BUFFER];
    memset(Buffer, 0, sizeof(char)*DEFAULT_BUFFER);
    for (auto g:groups) {
        strcat(Buffer, "id: ");
        char number[10];
        itoa(g.first, number, 10);
        strcat(Buffer, number);
        strcat(Buffer, "\tnames: ");
        for (auto i:g.second) {
            strcat(Buffer, socket_username[i]);
            strcat(Buffer, " ");
        }
        strcat(Buffer, "\n");
    }
//    Buffer[strlen(Buffer)] = '\0';
    b.m.message = Buffer;
    b.m.length = strlen(Buffer);

    return b;
}

opcode stringToOp(string s){
    if (s == "name") return user_registration;
    if (s == "join") return join;
    if (s == "quit") return quit_group;
    if (s == "exit") return program_exit;
    return error;
}