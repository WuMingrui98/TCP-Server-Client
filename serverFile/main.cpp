//
// Created by Mingrui Wu on 2021/11/9.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <cassert>
#include "threadpool.h"
#include "tcp_conn.h"
#include "log.h"

using namespace std;
#define MAX_FD 65536   // 最大的文件描述符个数
#define MAX_EVENT_NUMBER 10000  // 监听的最大的事件数量
// 添加文件描述符
extern void addfd(int epollfd, int fd, bool one_shot);

extern void removefd(int epollfd, int fd);


void addsig(int sig, void( handler )(int)) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

int thread_number = 8; // 线程池大小
int max_requests = 10000; // 最大请求数


int main(int argc, char *argv[]) {
    // 初始化日志功能
    Log *log = Log::GetLogInstance();
    log->init("./serverLog", 100000);

    if (argc <= 1) {
        printf("usage: %s port_number\n", basename(argv[0]));
        return 1;
    }
    int port = atoi(argv[1]);
    if (argv[2] != nullptr) {
        thread_number = atoi(argv[2]);
    }
    addsig(SIGPIPE, SIG_IGN);

    // 初始化线程池
    threadpool<tcp_conn> *pool = nullptr;
    try {
        pool = new threadpool<tcp_conn>(thread_number, max_requests);
    } catch (...) {
        return 1;
    }
    auto *users = new tcp_conn[MAX_FD];

    // 设置监听套接字
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);

    int ret = 0;
    struct sockaddr_in address{};
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    // 端口复用
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ret = bind(listenfd, (struct sockaddr *) &address, sizeof(address));
    if (ret == -1) {
        perror("bind");
        return -1;
    }
    ret = listen(listenfd, 5);
    if (ret == -1) {
        perror("listen");
        return -1;
    }
    // 创建epoll对象，和事件数组，添加
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    // 添加到epoll对象中
    addfd(epollfd, listenfd, false);
    tcp_conn::m_epollfd = epollfd;

    // epoll监控套接字
    while (true) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address{};
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *) &client_address, &client_addrlength);
                if (connfd < 0) {
                    perror("accept");
                    continue;
                }
                if (tcp_conn::m_user_count >= MAX_FD) {
                    close(connfd);
                    continue;
                }
                users[connfd].init(connfd, client_address);
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                users[sockfd].close_conn();
            } else if (events[i].events & EPOLLIN) {
                if (users[sockfd].read()) {
                    pool->append(users + sockfd);
                } else {
                    users[sockfd].close_conn();
                }
            } else if (events[i].events & EPOLLOUT) {
                users[sockfd].write();
                users[sockfd].close_conn();
            }
        }
    }
    close(epollfd);
    close(listenfd);
    delete[] users;
    delete pool;
    return 0;
}
