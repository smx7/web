#pragma once 

#include<iostream>
#include<string>
#include<unistd.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<strings.h>
#include<sstream>
#include<unordered_map>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<fcntl.h>
#include<errno.h>
#include"Log.hpp"


#define OK 200
#define NOT_FOUND 404

#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define HTTP_VERSION "http/1.0"

std::unordered_map<std::string, std::string> suffix_map
{
    {".html","text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/x-javascript"}
};

class Protocol{
    public:
        static void MakeKV(std::unordered_map<std::string,std::string> &_kv,std::string &_str)
        {
            size_t pos=_str.find(": ");
            if(std::string::npos!=pos)
            {
                std::string _k=_str.substr(0,pos);
                std::string _v=_str.substr(pos+2);
                _kv.insert(make_pair(_k,_v));
            }
        }
        static std::string IntToString(int code)
        {
            std::stringstream ss;
            ss<<code;
            return ss.str();
        }
        static std::string CodeToDesc(int code)
        {
            switch(code)
            {
                case 200:
                    return "OK"; 
                case 404:
                    return "NOT_FOUND";
                default:
                    return "UNKNOW";
            }
        }
        static std::string SuffixToType(const std::string &_suffix)
        {
            return suffix_map[_suffix];
        }
};
class Request{
    public:
        std::string rq_line;
        std::string rq_head;
        std::string blank;
        std::string rq_text;

        std::string method;
        std::string uri;
        std::string version;

        bool cgi;

        std::string path;
        std::string param;

        int resource_size;
        std::string resource_suffix;
        std::unordered_map<std::string,std::string> head_kv;
        int content_length;

        Request():blank("\n"),cgi(false),path(WEB_ROOT)
    {}
        void RequestLineParse()
        {
            std::stringstream ss(rq_line);
            ss>>method>>uri>>version;
        }
        void UriParse()
        {   if(strcasecmp(method.c_str(),"GET")==0)
            {
                size_t pos=uri.find('?');
                if(std::string::npos!=pos)
                {
                    cgi=true;
                    path+=uri.substr(0,pos);
                    param=uri.substr(pos+1);
                }
                else
                {
                    path+=uri;
                }
            }
            else
            {
                path+=uri;
            }
            if(path[path.size()-1]=='/')
            {
                path+=HOME_PAGE;
            }
        }

        bool IsMethodLegal()
        {
            if(strcasecmp(method.c_str(),"GET")==0)
            {
                return true;
            }
            if(strcasecmp(method.c_str(),"POST")==0)
            {
                cgi=true;
                return true;
            }
            return false;
        }
        bool IsPathLegal()
        {
            struct stat st;
            if(stat(path.c_str(),&st)<0)    // stat() 将path的文件信息保存在结构体变量st中
            {
                std::cout<<path<<std::endl;
                LOG(WARNING,"path not found!");
                perror("errno");
                return false;
            }
            if(S_ISDIR(st.st_mode))     // S_ISDIR 判断是否为目录 st_mode表示文件类型和文件权限
            {
                path+="/";
                path+=HOME_PAGE;
            }
            else
            {
                if((st.st_mode & S_IXUSR ) || (st.st_mode & S_IXGRP) ||(st.st_mode & S_IXOTH)) //请求资源存在并可执行
                {
                    cgi = true;
                }
            }
            resource_size=st.st_size;
            size_t pos=path.rfind('.');
            if(std::string::npos!=pos)
            {
                resource_suffix=path.substr(pos); //提取资源后缀
            }
            return true;
        }
        bool RequestHeadParse()
        {
            int start=0;
            while(start<rq_head.size())
            {
                size_t pos=rq_head.find('\n',start);
                if(std::string::npos==pos)
                {
                    return false;
                }

                std::string one_head=rq_head.substr(start,pos-start);
                if(!one_head.empty())
                {
                    Protocol::MakeKV(head_kv,one_head);
                }
                else
                {
                    break;
                }
                start=pos+1;
            }
            return true;
        }

        int GetContentLength()
        {
            std::string len=head_kv["Content-Length"];
            if(!len.empty())
            {
                std::stringstream ss(len);
                ss>>content_length;
            }
            return content_length;
        }
        bool IsNeedRecvText()
        {
            if(strcasecmp(method.c_str(),"POST")==0)
            {
                return true;
            }
            return false;
        }
        bool IsCgi()
        {
            return cgi;
        }
        void SetResourceSize(size_t _rs)
        {
            resource_size=_rs;
        }

        ~Request()
        {}
};

class Response
{
    public:
        std::string rsp_line;
        std::string rsp_head;
        std::string blank;
        std::string rsp_text;

        int code;
        int fd;

        Response():blank("\n"),code(OK),fd(-1)
    {}
        void MakeResponseFirstLine()
        {
            rsp_line=HTTP_VERSION;
            rsp_line+=" ";
            rsp_line+=Protocol::IntToString(code);
            rsp_line+=" ";
            rsp_line+= Protocol::CodeToDesc(code);
            rsp_line+="\n";
        }
        void MakeResponseHead(Request *&_rq)
        {
            rsp_head="Content-Length: ";
            rsp_head+=Protocol::IntToString(_rq->resource_size);
            rsp_head+="\n";
            rsp_head+="Content-Type: ";
            rsp_head+=Protocol::SuffixToType(_rq->resource_suffix);
            rsp_head+="\n";
        }
        void OpenResource(Request *&_rq)
        {
            fd=open(_rq->path.c_str(),O_RDONLY);
        }
        ~Response()
        {
            if(fd>=0)
            {
                close(fd);
            }
        }
};



class Connect
{
    private:
        int sock;
    public:
        Connect(int _sock):sock(_sock)
    {}
        int RecvOneLine(std::string& _line) // 获取一行信息
        {
            char c='X';
            while(c!='\n')
            {
                ssize_t s=recv(sock,&c,1,0);
                if(s>0)
                {
                    if(c=='\r')
                    {
                        recv(sock,&c,1,MSG_PEEK);
                        if(c=='\n')
                        {
                            recv(sock,&c,1,0);
                        }
                        else
                        {
                            c='\n';
                        }
                    }
                    _line.push_back(c);
                }
                else
                {
                    break;
                }
            }
            return _line.size();
        }
        void RecvRequestHead(std::string &_head) //获取全部协议头 包括空行
        {
            _head="";
            std::string _line="";
            while(_line!="\n")
            {
                _line="";
                RecvOneLine(_line);
                _head+=_line;
            }
        }
        void RecvRequestText(std::string &_text,int _len,std::string &_param)
        {
            char c;
            int i=0;
            while(i<_len)
            {
                recv(sock,&c,1,0);
                _text.push_back(c);
                i++;

            }
            _param =_text;
        }
        void SendResponse(Request *&_rq,Response *&_rsp,bool _cgi)
        {
            std::string &_rsp_line=_rsp->rsp_line;
            std::string &_rsp_head=_rsp->rsp_head;
            std::string &_blank=_rsp->blank;
            send(sock, _rsp_line.c_str(), _rsp_line.size(), 0);
            send(sock, _rsp_head.c_str(), _rsp_head.size(), 0);
            send(sock, _blank.c_str(), _blank.size(), 0);

            if(_cgi)
            {
                std::string &_rsp_text = _rsp->rsp_text;
                send(sock,_rsp_text.c_str(), _rsp_text.size(), 0);
            }
            else
            {
                int &fd = _rsp->fd;
                sendfile(sock, fd, NULL,_rq->resource_size);
            }
        }
        ~Connect()
        {
            if(sock>=0)
            {
                close(sock);
            }
        }
};

class Entry
{
    public:
        static void ProcessNonCgi(Connect *&_conn, Request *&_rq, Response *&_rsp)
        {
            int &_code = _rsp->code;
            _rsp->MakeResponseFirstLine();
            _rsp->MakeResponseHead(_rq);
            _rsp->OpenResource(_rq);
            _conn->SendResponse(_rq, _rsp, false);
        }
        static void ProcessCgi(Connect *&_conn,Request *&_rq,Response *&_rsp)
        {
            int input[2];
            int output[2];
            std::string &_param=_rq->param;
            std::string &_rsp_text=_rsp->rsp_text;

            pipe(input);
            pipe(output);

            pid_t pid=fork();
            if(pid<0)
            {
                _rsp->code=NOT_FOUND;
                LOG(ERROR,"fork error");
                return ;
            }
            else if(pid==0)
            {
                close(input[1]);
                close(output[0]);

                const std::string &_path=_rq->path;
                std::string cl_env="Content-Length=";
                cl_env += Protocol::IntToString(_param.size());
                putenv((char*)cl_env.c_str());

                dup2(input[0], 0);
                dup2(output[1], 1);
                execl(_path.c_str(), _path.c_str(), NULL);
                exit(1);
            }
            else
            {
                close(input[0]);
                close(output[1]);

                size_t size=_param.size();
                const char *p=_param.c_str();
                size_t total=0;
                size_t cur=0;

                while(total<size&&(\
                            cur=write(input[1],p+total,size-total)>0\
                            ))
                {
                    total+=cur;
                }
                
                char c;
                while(read(output[0],&c,1)>0)
                {
                    _rsp_text.push_back(c);
                }
                close(input[1]);
                close(output[0]);

                _rsp->MakeResponseFirstLine();
                _rq->SetResourceSize(_rsp_text.size());
                _rsp->MakeResponseHead(_rq);

                _conn->SendResponse(_rq,_rsp,true);


            }
        }
        static int  ProcessResponse(Connect *&_conn, Request *&_rq, Response *&_rsp)
        {
            if(_rq->IsCgi())
            {
                ProcessCgi(_conn, _rq, _rsp);
            }
            else
            {
                ProcessNonCgi(_conn, _rq, _rsp);
            }
        }
        static int HandlerRequest(int _sock)
        {

            LOG(INFO,"start HandlerRequest");
           // int _sock=*(int*)_arg;
           // delete (int*)_arg;
            Connect*  _conn=new Connect(_sock);
            Request* _rq=new Request();
            Response* _rsp=new Response();

            int &_code=_rsp->code;

            _conn->RecvOneLine(_rq->rq_line);//获取请求首行
            LOG(INFO,"recv first line done");
            _rq->RequestLineParse();//解析请求首行
            LOG(INFO,"parse first line done");

            if(!_rq->IsMethodLegal()) //判断请求方法是否合法
            {
                _code=NOT_FOUND;
                goto end;
            }
            LOG(INFO,"method is legal");

            _rq->UriParse();//请求方法合法 解析URI
            LOG(INFO,"parse uri done");

            if(!_rq->IsPathLegal()) //判断资源路径是否合法
            {
                _code=NOT_FOUND;
                goto end;
            }
            LOG(INFO,"path is legal");
            _conn->RecvRequestHead(_rq->rq_head);  //获取协议头信息
            if(_rq->RequestHeadParse())       //解析协议头
            {
                LOG(INFO,"parse head is finished");
            }
            else
            {
                _code=NOT_FOUND;
                goto end;
            }
            if(_rq->IsNeedRecvText())
            {
                _conn->RecvRequestText(_rq->rq_text,_rq->GetContentLength(),_rq->param);
            }
            LOG(INFO,"request recv is finished");
            ProcessResponse(_conn,_rq,_rsp);

end:
            if(_code!=OK)
            {

            }
            delete _conn;
            delete _rq;
            delete _rsp;


        }
};




