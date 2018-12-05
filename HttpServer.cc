#include"HttpServer.hpp"


static void Usage(std::string proc_)
{
    std::cout<<"Usage: "<<proc_<<"port"<<std::endl;
}


int main(int argc,char* argv[])
{
    if(argc!=2)
    {
        Usage(argv[0]);
        exit(1);
    }

    HttpdServer* serp=new HttpdServer(atoi(argv[1]));
    serp->InitServer();
    serp->Start();

    delete serp;
    return 0;
}
