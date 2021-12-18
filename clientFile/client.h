#ifndef _CLIENT_H
#define _CLIENT_H
#include<stdio.h>
#include<assert.h>
#include<openssl/evp.h>
#include<openssl/md5.h>
#include<sys/epoll.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/time.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<pthread.h>
#include<syslog.h>
#include<arpa/inet.h>
#include<errno.h>
#include <sys/wait.h>
#include<vector>
#include<algorithm>
using namespace std; 

void err(const char* message);

double time_count(timeval t_start, timeval t_end);

int localtime2_usec(char* message);

void setnonblocking(int fd);
#endif
// !_CLIENT_H
