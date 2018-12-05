#pragma once

#include<iostream>
#include<string>
#include<sys/time.h>

#define INFO 0
#define DEBUG 1
#define WARNING 2
#define ERROR 3

uint64_t GetTimeStamp()
{
    struct timeval time; 
    gettimeofday(&time, NULL);
    return time.tv_sec;
}


std::string GetLogLevel (int level)
{
    switch(level)
    {
        case 0:
            return "INFO";
        case 1:
            return "DEBUG";
        case 2:
            return "WARNING";
        case 3:
            return "ERROR";
        default:
            return "UNKNOW";
    }
}

void Log(int level,std::string message,std::string file,int line)
{
    std::cout<<"[ "<<GetTimeStamp()<<" ]"<<" ["<<GetLogLevel(level)<<" ]"<<" ["<<file<<" : "<<line<<" ] "<<message<<std::endl;
}

#define LOG(level,message) Log(level,message,__FILE__,__LINE__)



