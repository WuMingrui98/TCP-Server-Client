#ifndef _LOG_
#define _LOG_

#include"lock.h"
#include<cstring>
#include<mutex>
#include<list>
#include<thread>
#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include <unistd.h>


//异步写日志，即先把日志消息写入消息缓冲区，然后从缓冲区取写入到文件
class Log {
private:
    //从缓冲队列取日志消息
    void async_write_log();

    // 生成随机id
    static char *random_uuid( char buf[37] );

    // 日志类销毁
    void destroy();

//public:
    static semphore m_fullBuff, m_emptyBuff;
    int m_max_task;
    // 需要写入的文件
    FILE *m_fp;
    char m_log_name[128];
    // 互斥量
    static std::mutex m_mutex;
    // 保存单例Log对象
    static Log *m_log;
    static std::list<std::string> m_list;
    std::thread log_thread;
    bool isRun;

private:
    // 为什么要设置为private因为是Log类是单例的，不能再类外面实例化
    Log() : isRun(true) {
        static CGarbo Garbo;
    }

    class CGarbo
    {
    public:
        ~CGarbo() {
            if (Log::m_log)
            {
                while (!Log::m_list.empty()) {
                    sleep(1);
                }
                if (Log::m_log->m_fp) Log::m_log->destroy();
                m_fullBuff.V();
                delete Log::m_log;
                Log::m_log = nullptr;
            }
        }
    };

    // 禁止这种方式实例化Log对象
    Log(Log &tmp) = delete;

    // 禁用拷贝构造函数
    Log &operator=(Log &tmp) = delete;

public:
    ~Log() {
        std::cout<<"异步日志退出了！！"<<std::endl;
    }

public:
    static Log *GetLogInstance();



//    template<typename ...Args>
    static void INFO(char *clientIP, const char *msg) {
        m_emptyBuff.P();
        m_mutex.lock();

        char log_buf[10240] = {0};

        struct timeval tv{};
        struct timezone tz{};

        gettimeofday(&tv, &tz);
        struct tm *pTime;
        pTime = localtime(&tv.tv_sec);

        sprintf(log_buf, "[%d:%d:%d %d:%d:%d.%ld] ", pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday,
                pTime->tm_hour, pTime->tm_min, pTime->tm_sec, tv.tv_usec / 1000);
        int n = strlen(log_buf);
        char guid[37];
        random_uuid(guid);
        sprintf(log_buf + n, " \"recv %s msg,msgid:%s ,msg content: %s \n", clientIP, guid, msg);
        std::string s(log_buf);

        m_list.push_back(std::move(s));

        m_mutex.unlock();
        m_fullBuff.V();
    }

    void init(const char *file_name, int max_queue_size);
};


#endif

