//
// Created by 'Confidence'F on 10/21/2022.
//

#include <iomanip>
#include "prettyPrint.h"

void prettyPrint::pretty(Task t) {
    time_t result = std::time(nullptr);
    cout<<"[" << asctime(localtime(&result))<<"\t";
    cout<<"From: "<<t.b.m.user_name<<"]";
    cout<<t.b.m.message<<endl;
}

void prettyPrint::pretty(bean b) {
    time_t tt = b.m.time;
    auto* ti = localtime(&tt);
    cout<<"[" << put_time(ti, "%c")<<"\t";
    cout<<"From: "<<b.m.user_name<<"]";
//    cout<<b.m.message<<endl;
    printf("%s\n", b.m.message);
}
