//
// Created by 'Confidence'F on 10/21/2022.
//

#include <iomanip>
#include "prettyPrint.h"

//void prettyPrint::pretty(Task t) {
//    time_t result = std::time(nullptr);
//    cout<<"[" << asctime(localtime(&result))<<"\t";
//    cout<<"From: "<<t.b.m.user_name<<"]";
//    cout<<t.b.m.message<<endl;
//}
//
//void prettyPrint::pretty(bean b) {
//    time_t tt = b.m.time;
//    auto* ti = localtime(&tt);
//    cout<<"[" << put_time(ti, "%c")<<"\t";
//    cout<<"From: "<<b.m.user_name<<"]";
////    cout<<b.m.message<<endl;
//    printf("%s\n", b.m.message);
//}
void prettyPrint::pretty(condition c, long long currtime, unsigned int seq, unsigned short checksum) {

    auto* currt = localtime(&currtime);
//    auto* beforet = localtime(&timelast);
    timelast = currtime;

    string cond;
    switch (c) {
        case ack:
            cond = "ACK";
            break;
        case syn:
            cond = "SYN";
            break;
        case syn + ack:
            cond = "SYN + ACK";
            break;
        case fin + ack:
            cond = "FIN + ACK";
            break;
        default:
            cond = "";
    }
    cout<<"[" << put_time(currt, "%c")<<"\t";
    cout<<"Type: "<<cond<<"] ";
//    cout<<"Speed: "<< size / ((currtime - timelast) / 1000.0)<<"Bytes/sec ";
    cout<<"Seq: "<<std::dec<< seq<<" ";
    cout<<"Checksum: "<<std::hex<<checksum<<endl;

}

prettyPrint::prettyPrint(long long int timelast) : timelast(timelast) {}

void prettyPrint::outputSpeed(long long int currtime, long long int starttime, unsigned int size) {
    auto* currt = localtime(&currtime);
    cout<<"[" << put_time(currt, "%c")<<"\tSend Done]";
    cout<<"Speed: "<< size / ((currtime - starttime) / 1000.0)<<"Bytes/sec "<<endl;
}
