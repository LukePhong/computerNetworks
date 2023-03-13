//
// Created by 'Confidence'F on 10/20/2022.
//

#ifndef PROJ1_QUEUE_BLOCK_H
#define PROJ1_QUEUE_BLOCK_H

#include<iostream>
#include<queue>
#include<pthread.h>
#include<unistd.h>

class Task
{
public:
    int _x;
    int _y;
public:
    Task()
    {}
    Task(int x,int y):_x(x),_y(y)
    {}
    int Run()
    {
        return _x+_y;
    }
    ~Task()
    {}
};
class BlockQueue
{
private:
    std::queue<Task> q;
    size_t _cap;
    pthread_mutex_t lock;
    pthread_cond_t c_cond;//消费者的条件不满足时，将来消费者在该条件变量下等
    pthread_cond_t p_cond;//生产者的条件不满足时，将来生产者在该条件下等
public:
    bool IsFull()
    {
        return q.size() >= _cap;
    }
    bool IsEmpty()
    {
        return q.empty();
    }
    void LockQueue()
    {
        pthread_mutex_lock(&lock);
    }
    void UnLockQueue()
    {
        pthread_mutex_unlock(&lock);
    }
    void WakeUpComsumer()
    {
        pthread_cond_signal(&c_cond);
    }
    void WakeUpProductor()
    {
        pthread_cond_signal(&p_cond);
    }
    void ProducterWait()
    {
        pthread_cond_wait(&p_cond,&lock);
        //这里为什么要传锁，我们在等待时肯定是条件不满足了，我们通过判断才知道条件满不满足，
        //判断就需要保证进入临界区，我们是持有锁进入的，wait的时候必须要释放锁
        //在调用该函数的时候，自动会释放lock
        //当该函数被返回时，返回到了临界区内，所以，该函数会让该线程重新持有该锁
    }
    void ComsumerWait()
    {
        pthread_cond_wait(&c_cond,&lock);//在消费者释放锁时，生产者正申请锁，而消费者在等待
    }
public:
    BlockQueue(size_t cap):_cap(cap)
    {
        pthread_mutex_init(&lock,nullptr);
        pthread_cond_init(&c_cond,nullptr);
        pthread_cond_init(&p_cond,nullptr);
    }
    void Put(Task t)
    {
        LockQueue();
        //if(isFull())
        while(IsFull())
        {
            WakeUpComsumer();//唤醒消费者
            ProducterWait();//生产者等待
        }
        q.push(t);
        UnLockQueue();
    }
    void Get(Task& t)
    {
        LockQueue();
        //if(IsEmpty)
        while(IsEmpty())
        {
            WakeUpProductor();
            ComsumerWait();//消费者者等待
        }
        t = q.front();
        q.pop();
        UnLockQueue();
    }
    ~BlockQueue()
    {
        pthread_cond_destroy(&lock);
        pthread_cond_destroy(&c_cond);
        pthread_cond_destroy(&p_cond);
    }
};

#endif //PROJ1_QUEUE_BLOCK_H
