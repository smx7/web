#pragma once

#include<iostream>
#include<queue>
#include<pthread.h>
#include"Log.hpp"

#define NUM 5

typedef int(*handler_t)(int);

class Task
{

    private:
        int sock;
        handler_t handler;
    public:
        Task()
        {
            sock=-1;
            handler=NULL;
        }
        void SetTask(int _sock,handler_t _handler)
        {
            sock=_sock;
            handler=_handler;
        }
        void Run()
        {
            handler(sock);
        }
};
class ThreadPool
{
    private:
        std::queue<Task> task_queue;
        int thread_total_num;
        int thread_idle_num;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        volatile bool is_quit;

    public:
        void PushTask(Task &_t)
        {
            pthread_mutex_lock(&mutex);
            if(is_quit)
            {
                pthread_mutex_unlock(&mutex);
                return ;
            }
            task_queue.push(_t);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);

        }
        void PopTask(Task &_t)
        {
            _t=task_queue.front();
            task_queue.pop();
        }
        void Stop()
        {
            pthread_mutex_lock(&mutex);
            is_quit=true;
            pthread_mutex_unlock(&mutex); 
            while(thread_idle_num>0)
            {
                pthread_cond_broadcast(&cond);
            }
        }
        void ThreadIdle()
        {
            if(is_quit)
            {
                pthread_mutex_unlock(&mutex);
                thread_total_num--;
                LOG(INFO,"thread quit");
                pthread_exit((void*)0);
            }
            thread_idle_num++;
            pthread_cond_wait(&cond,&mutex);
            thread_idle_num--;

        }
        static void* ThreadRoutine(void* arg)
        {
            ThreadPool* tp=(ThreadPool*)arg;
            pthread_detach(pthread_self());
            while(1)
            {
                pthread_mutex_lock(&(tp->mutex));
                while((tp->task_queue).empty())
                {
                    tp->ThreadIdle();
                }
                Task t;
                tp->PopTask(t);
                pthread_mutex_unlock(&(tp->mutex));
                LOG(INFO,"task has been token");
                std::cout<<"thread id is:"<<pthread_self()<<std::endl;
                t.Run();
            }
        }
        ThreadPool()
        {
            thread_total_num=NUM;
            thread_idle_num=0;
            is_quit=false;
            pthread_mutex_init(&mutex,NULL);
            pthread_cond_init(&cond,NULL);
        }
        void InitThreadPool()
        {
            for(int i=0;i<thread_total_num;i++)
            {
                pthread_t tid;
                pthread_create(&tid,NULL,ThreadRoutine,this);
            }
        }
        ~ThreadPool()
        {
            pthread_mutex_destroy(&mutex);
            pthread_cond_destroy(&cond);
        }

};
