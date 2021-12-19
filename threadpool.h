#include <pthread.h>

typedef struct ThreadPool ThreadPool;

ThreadPool* poolInit(int, int, int);
void* manage(void*);
void* work(void*);
void threadExit(ThreadPool*);

// 定义任务结构体
typedef struct Task {
	void* (*func)(void* arg);
	void* arg;
}Task;

// 定义线程池结构体
struct ThreadPool {
	// 任务队列
	Task* taskQueue;
	int queueCapacity;	// 队列中能容纳的最大任务数
	int queueSize;	// 队列中任务的实际个数
	int queueHead;	// 队头
	int queueTail;	// 队尾

	// 管理者线程
	pthread_t managerID;

	// 工作的线程
	pthread_t* workersID;
	int minNum;		// 最小线程数
	int maxNum;		// 最大线程数
	int livingNum;	// 存活的线程
	int workingNum;	// 正在工作的线程
	int exitNum;	// 要销毁的线程

	pthread_mutex_t mutexPool;			// 锁整个线程池
	pthread_mutex_t mutexWorkingNum;	// 锁workingNum变量
	pthread_cond_t notEmpty;	// 任务队列不满
	pthread_cond_t notFull;		// 任务队列不空

	int shutdown;	// 要销毁线程池为1，不要销毁为0
};
