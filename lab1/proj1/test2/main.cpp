//
// Created by 'Confidence'F on 10/20/2022.
//
#include "queue_block.h"
using namespace std;

pthread_mutex_t c_lock;
pthread_mutex_t p_lock;
void* r1(void* arg)
{
    //生产者
    BlockQueue* bq =(BlockQueue*)arg;
    while(true)
    {
        pthread_mutex_lock(&p_lock);
        int x = rand()%10+1;
        int y = rand()%100+1;
        Task t(x,y);
        bq->Put(t);
        pthread_mutex_unlock(&p_lock);
        cout<<"Product Task is: "<<x<<'+'<<y<<"= ?"<<endl;
    }
}
void* r2(void* arg)
{
    //消费者
    BlockQueue* bq = (BlockQueue*)arg;
    while(true)
    {
        //取数据
        Task t;
        pthread_mutex_lock(&c_lock);
        bq->Get(t);
        pthread_mutex_unlock(&c_lock);
        cout<<"Consumer: "<<t._x<<"+"<<t._y<<"="<<t.Run()<<endl;
    }
}
int main()
{

    BlockQueue* bq = new BlockQueue(5);
    pthread_t c,p;
    pthread_mutex_init(&p_lock,NULL);
    pthread_mutex_init(&c_lock,NULL);
    pthread_create(&p,NULL,r1,(void*)bq);
    pthread_create(&p,NULL,r1,(void*)bq);
    pthread_create(&p,NULL,r1,(void*)bq);
    pthread_create(&p,NULL,r1,(void*)bq);

    pthread_create(&c,NULL,r2,(void*)bq);
    pthread_create(&c,NULL,r2,(void*)bq);
    pthread_create(&c,NULL,r2,(void*)bq);
    pthread_create(&c,NULL,r2,(void*)bq);
    pthread_create(&c,NULL,r2,(void*)bq);

    pthread_join(p,nullptr);
    pthread_join(c,nullptr);

    pthread_mutex_destroy(&c_lock);
    pthread_mutex_destroy(&p_lock);
    delete bq;
    return 0;
}