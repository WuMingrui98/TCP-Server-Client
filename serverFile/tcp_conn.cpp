#include <valarray>
#include "tcp_conn.h"
#include "log.h"


// 设置文件描述符非阻塞
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 向epoll中添加需要监听的文件描述符
void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;
    if (one_shot) {
        // 防止同一个通信被不同的线程处理
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // 设置文件描述符非阻塞
    setnonblocking(fd);
}

// 从epoll中移除监听的文件描述符
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// 修改文件描述符，重置socket上的EPOLLONESHOT事件，以确保下一次可读时，EPOLLIN事件能被触发
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 所有的客户数
int tcp_conn::m_user_count = 0;
// 所有socket上的事件都被注册到同一个epoll内核事件中，所以设置成静态的
int tcp_conn::m_epollfd = -1;
// 回复的消息
char tcp_conn::response[] = "res:Hello,TCP!";

// 关闭连接
void tcp_conn::close_conn() {
    if (m_sockfd != -1) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--; // 关闭一个连接，将客户总数量-1
    }
}

// 初始化连接,外部调用初始化套接字地址
void tcp_conn::init(int sockfd, const sockaddr_in &addr) {
    m_sockfd = sockfd;
    m_address = addr;

    // 端口复用
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    addfd(m_epollfd, sockfd, true);
    m_user_count++;
    init();
}

void tcp_conn::init() {
    m_read_idx = 0;
    bzero(m_read_buf, READ_BUFFER_SIZE);
    bzero(m_write_buf, READ_BUFFER_SIZE);
}

// 循环读取客户数据，直到无数据可读或者对方关闭连接
bool tcp_conn::read() {
    if (m_read_idx >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytes_read = 0;
    while (true) {
        // 数据读到了指向m_read_buf的char数组中
        // 从m_read_buf + m_read_idx索引出开始保存数据，大小是READ_BUFFER_SIZE - m_read_idx
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx,
                          READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 没有数据
                break;
            }
            return false;
        } else if (bytes_read == 0) {   // 对方关闭连接
            return false;
        }
        m_read_idx += bytes_read;
    }
    return true;
}

// 写响应
bool tcp_conn::write() const {

    int left = sizeof(response);
//    int nwrite = 0;
//    const char* p = response;
//    while (left > 0) {
//        if ((nwrite = send(m_sockfd, response, left, 0)) > 0) {
//            p += nwrite;
//            left -= nwrite;
//        } else if(nwrite == -1) {
//            return false;
//        }
//    }
    int ret = send(m_sockfd, response, left, 0);
    if (ret == -1) {
        perror("send");
        return false;
    }

    return true;
}


// 由线程池中的工作线程调用，这是处理HTTP请求的入口函数
void tcp_conn::process() {
    // 写日志
    char ip[32];
    inet_ntop(AF_INET, &m_address.sin_addr.s_addr, ip, sizeof(ip));
    Log::INFO(ip, m_read_buf);
    // 将m_sockfd置为EPOLLOUT
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}