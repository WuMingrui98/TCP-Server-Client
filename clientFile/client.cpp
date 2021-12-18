#include "client.h"
#define log_message_size 200
#define BUF_SZ 100

void err(const char* message)
{
	perror(message);
	exit(1);
}

int localtime2_usec(char* message)
{
	struct timeval tv;
	struct timezone tz;
	struct tm* t;
	gettimeofday(&tv, &tz);
	t = localtime(&tv.tv_sec);
	return snprintf(message, log_message_size - 1, "%02d:%02d:%02d.%ld", t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec / 1000);
}
double time_count(timeval t_start, timeval t_end)
{
	double a=((double)t_end.tv_usec - (double)t_start.tv_usec) / 1000;
	double delta_t = (t_end.tv_sec - t_start.tv_sec) * 1000 + a;
	return delta_t;
}

void setnonblocking(int fd)
{
	int flag = fcntl(fd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flag);
}

