#include "threadpool.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

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

ThreadPool* pool_create(int maxCapacity, int maxThreadNum, int minThreadNum) {
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));	
	do {
		if (!pool) {
			printf("malloc pool failed...\n");
			break;
		}

		pool->workersID = (pthread_t*)malloc(sizeof(pthread_t) * maxThreadNum);
		if (pool->workersID == NULL) {
			printf("malloc workersID failed...\n");
			break;
		}

		memset(pool, 0, sizeof(pthread_t) * maxThreadNum);
		pool->minNum = minThreadNum;
		pool->maxNum = maxThreadNum;
		pool->livingNum = minThreadNum;
		pool->workingNum = 0;
		pool->exitNum = 0;

		if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
			pthread_mutex_init(&pool->mutexWorkingNum, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0) {
			printf("mutex or cond init failed...\n");
			break;
		}

		// 任务队列	
		pool->taskQueue = (Task*)malloc(sizeof(Task) * maxCapacity);
		pool->queueCapacity = maxCapacity;
		pool->queueSize = 0;
		pool->queueHead = 0;
		pool->queueTail = 0;

		pool->shutdown = 0;

		// 创建线程
		pthread_create(&pool->managerID, NULL, manage, NULL);
		for (int i = 0; i < minThreadNum; i ++ )
			pthread_create(&pool->workersID[i], NULL, work, NULL);
		
		return pool;
	} while (0);

	if (pool && pool->workersID) free(pool->workersID);
	if (pool && pool->taskQueue) free(pool->taskQueue);
	if (pool) free(pool);
	return NULL;
}
