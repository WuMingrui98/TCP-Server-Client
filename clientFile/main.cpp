#include "client.h"
#define log_message_size 200
char filename[11] = "client_log";
int tcp_num;
int num;
char *content;
int sent;
int port;
char*ip;
pthread_mutex_t tcp_mutex;
pthread_mutex_t sent_mutex;
std::vector<double> counter;
void* task_handle(void* arg)
{
	char* start_time = (char*)malloc(100);
	char* end_time = (char*)malloc(100);
	memset(start_time, 0, 100);
	memset(end_time, 0, 100);
    timeval old_time;
    timeval now_time;
    gettimeofday(&old_time,NULL);
	localtime2_usec(start_time);
	while (1)
	{
		if (num == 0)
			break;
		usleep(20000);
	}
	localtime2_usec(end_time);
    gettimeofday(&now_time,NULL);
    double sec_count=time_count(old_time,now_time)/1000;
    double qps=(double)sent/sec_count;
	sort(counter.begin(),counter.end());
	printf("test start time:%s\n", start_time);
	printf("test end time:%s\n", end_time);
	printf("qps:%.2f/sec\n", qps);
	printf("Percentage of the requests served within a certain time(ms)\n");
	printf("P50:%.4fms\n", counter[sent * 0.5 - 1]);
	printf("P60:%.4fms\n", counter[sent * 0.6 - 1]);
	printf("P70:%.4fms\n", counter[sent * 0.7 - 1]);
	printf("P80:%.4fms\n", counter[sent * 0.8 - 1]);
	printf("P90:%.4fms\n", counter[sent * 0.9 - 1]);
    printf("P100:%.4fms\n",counter[sent-1]);
	free(start_time);
	free(end_time);
	return NULL;
}
void* thread_handle(void* arg)
{
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	timeval old_time;
	timeval now_time;
	while(1)
	{
		gettimeofday(&old_time, NULL);
		pthread_mutex_lock(&tcp_mutex);
		if (tcp_num==0)
		{
			num--;
			pthread_mutex_unlock(&tcp_mutex);
			break;
		}
		tcp_num--;
		pthread_mutex_unlock(&tcp_mutex);
		int fd = socket(PF_INET, SOCK_STREAM, 0);
		if (fd < 0)
			err("socket");
		if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		{
			usleep(500);
            continue;
		}
		int ret = 0;
        ret=write(fd, content, strlen(content));
		if (ret == -1)
			continue;
		gettimeofday(&now_time, NULL);
		pthread_mutex_lock(&sent_mutex);
		counter[sent++] = time_count(old_time, now_time);
		pthread_mutex_unlock(&sent_mutex);
		usleep(500);
		close(fd);
	}
	return NULL;
}


int main(int argc,char**argv)
{
    assert(argc == 6);
	pthread_mutex_init(&tcp_mutex, NULL);
	pthread_mutex_init(&sent_mutex, NULL);
	num = atoi(argv[1]);
	tcp_num = atoi(argv[2]);
	counter = std::vector<double>(tcp_num+1,100000.0);
	content = argv[3];
    ip=argv[4];
    port=atoi(argv[5]);
	pthread_t ThreadID[num];
	pthread_t task;
	pthread_create(&task, NULL, task_handle, NULL);
	usleep(1000);
	for (int i = 0;i < num;i++)
		pthread_create(&ThreadID[i], NULL, thread_handle, NULL);
	pthread_join(task, NULL);
	for (int i = 0;i < num;i++)
		pthread_join(ThreadID[i],NULL);
	pthread_mutex_destroy(&tcp_mutex);
	pthread_mutex_destroy(&sent_mutex);
	return 0;
}
