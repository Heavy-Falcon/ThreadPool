#include "threadpool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
		pthread_create(&pool->managerID, NULL, manage, pool);
		for (int i = 0; i < minThreadNum; i ++ )
			pthread_create(&pool->workersID[i], NULL, work, NULL);
		
		return pool;
	} while (0);

	if (pool && pool->workersID) free(pool->workersID);
	if (pool && pool->taskQueue) free(pool->taskQueue);
	if (pool) free(pool);
	return NULL;
}

void* work(void* arg) {
	ThreadPool* pool = (ThreadPool*)arg;

	while (1) {
		pthread_mutex_lock(&pool->mutexPool);
		// 如果任务队列为空，阻塞work线程
		while (!pool->shutdown && pool->queueSize == 0) {
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
		}

		if (pool->shutdown) {
			pthread_mutex_unlock(&pool->mutexPool);
			pthread_exit(NULL);
		}
		
		// 从任务队列中取一个任务
		Task task;
		task.func = pool->taskQueue[pool->queueHead].func;
		task.arg = pool->taskQueue[pool->queueHead].arg;
		pool->queueHead = (pool->queueHead + 1) % pool->queueCapacity;
		pool->queueSize -- ;

		pthread_mutex_unlock(&pool->mutexPool);

		pthread_mutex_lock(&pool->mutexWorkingNum);
		pool->workingNum ++ ;
		pthread_mutex_unlock(&pool->mutexWorkingNum);

		task.func(task.arg);
		free(task.arg);
		task.arg = NULL;

		pthread_mutex_lock(&pool->mutexWorkingNum);
		pool->workingNum -- ;
		pthread_mutex_unlock(&pool->mutexWorkingNum);

	}
	return NULL;
}
