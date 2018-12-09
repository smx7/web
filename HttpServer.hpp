#pragma once
#include<pthread.h>
#include "ProtocolUtil.hpp"
#include "ThreadPool.hpp"
#include"Log.hpp"
class HttpdServer{
    private:
        int listen_sock;
        int port;
        ThreadPool* tp;
    public:
        HttpdServer(int port_):port(port_),listen_sock(-1),tp(NULL)
        {}

        void InitServer()
        {
            listen_sock=socket(AF_INET,SOCK_STREAM,0);
            if(listen_sock<0)
            {
                LOG(ERROR,"Creat Socket Error!");
                exit(2);
            }

            int opt=1;
            setsockopt(listen_sock,SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            struct sockaddr_in local_addr;
            local_addr.sin_family=AF_INET;
            local_addr.sin_port=htons(port);
            local_addr.sin_addr.s_addr=INADDR_ANY;
            
            if(bind(listen_sock,(struct sockaddr*)&local_addr,sizeof(local_addr))<0)
            {
                LOG(ERROR,"Bind Socket Error!");
                exit(3);
            }

            if(listen(listen_sock,5)<0)
            {
                LOG(ERROR,"Listen Socket Error");
                exit(4);
            }
            LOG(INFO,"InitSever Success!");
            
            tp=new ThreadPool();
            tp->InitThreadPool();

        }
        void Start()
        {
            LOG(INFO,"Start Server!");
            while(1)
            {
                struct sockaddr_in sev_addr;
                socklen_t len=sizeof(sev_addr);
                int sock=accept(listen_sock,(struct sockaddr*)&sev_addr,&len);
                if(sock<0)
                {
                    LOG(WARNING,"Accept Error!");
                    continue;
                }
                LOG(INFO,"Get New Cilent,Create Thread.");
               // pthread_t tid;
               // int *sockp=new int;
               // *sockp=sock;
               // pthread_create(&tid,NULL,Entry::HandlerRequest,(void*)sockp);
                Task t;
                t.SetTask(sock,Entry::HandlerRequest);
                tp->PushTask(t);
            }
        }

        ~HttpdServer()
        {
            if(listen_sock!=-1)
            {
                close(listen_sock);
            }
            port=-1;

        }

};
