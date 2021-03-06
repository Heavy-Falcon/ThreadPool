#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct ThreadPool ThreadPool;
typedef struct Task Task;

ThreadPool* threadPoolInit(int maxCapacity, int maxThreadNum, int minThreadNum);
void* manage(void* arg);
void* work(void* arg);
void threadExit(ThreadPool* pool);
void threadPoolAdd(ThreadPool* pool, Task* task);
int threadPoolWorkingNum(ThreadPool* pool);
int threadPoolLivingNum(ThreadPool* pool);
int threadPoolDestroy(ThreadPool* pool);

// 定义任务结构体
struct Task {
	void* (*func)(void* arg);
	void* arg;
};

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
	pthread_cond_t empty;	// 任务队列不满
	pthread_cond_t full;		// 任务队列不空

	int shutdown;	// 要销毁线程池为1，不要销毁为0
};

#endif
