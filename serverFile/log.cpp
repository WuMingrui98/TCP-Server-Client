#include"log.h"

std::mutex Log::m_mutex;
Log *Log::m_log = nullptr;

semphore Log::m_fullBuff;
semphore Log::m_emptyBuff;
std::list<std::string> Log::m_list;

//创建日志单例类
Log* Log::GetLogInstance() {
    if(m_log == nullptr) {
        std::unique_lock<std::mutex> ulock(m_mutex);
        if(m_log == nullptr) {
            m_log = new Log;
            if(m_log == nullptr) return nullptr;
        }
    }
    return m_log;
}

//从缓冲队列取日志消息
void Log::async_write_log() {
    while(isRun) {
        m_fullBuff.P();
        if (!isRun) {break;}
        m_mutex.lock();
        auto item = m_list.front();
        m_list.pop_front();
        fputs(item.c_str(), m_fp);
        fflush(m_fp);
        m_mutex.unlock();
        m_emptyBuff.V();
    }
}

//初始化日志名，最大并发量
void Log::init(const char *file_name, int max_queue_size) {
    m_max_task = max_queue_size;
    memcpy(m_log_name, file_name, strlen(file_name) + 1);
    m_fp = fopen(file_name, "a");  //追加模式日志文件   
    m_fullBuff.setNum(0);           //设置信号量值
    m_emptyBuff.setNum(m_max_task); //设置信号量值
    log_thread = std::thread(&Log::async_write_log, this);
    log_thread.detach();
}

void Log::destroy() {
    isRun = false;
    std::unique_lock<std::mutex> ulock(m_mutex);
    if(m_fp) {
        fflush(m_fp);
        fclose(m_fp);
    }
    m_max_task = false;
    m_fp = nullptr;
}

char *Log::random_uuid(char *buf) {
    const char *c = "89ab";
    char *p = buf;
    int n;
    for( n = 0; n < 16; ++n )
    {
        int b = rand()%255;
        switch( n )
        {
            case 6:
                sprintf(p, "4%x", b%15 );
                break;
            case 8:
                sprintf(p, "%c%x", c[rand()%strlen(c)], b%15 );
                break;
            default:
                sprintf(p, "%02x", b);
                break;
        }

        p += 2;
        switch( n )
        {
            case 3:
            case 5:
            case 7:
            case 9:
                *p++ = '-';
                break;
        }
    }
    *p = 0;
    return buf;
}


