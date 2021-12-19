#include "threadpool.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define NUM 2

ThreadPool* threadPoolInit(int maxCapacity, int maxThreadNum, int minThreadNum) {
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));	
	do {
		if (pool == NULL) {
			printf("malloc pool failed...\n");
			break;
		}

		// 初始化每个线程的ID为0
		pool->workersID = (pthread_t*)malloc(sizeof(pthread_t) * maxThreadNum);
		if (pool->workersID == NULL) {
			printf("malloc workersID failed...\n");
			break;
		}
		memset(pool->workersID, 0, sizeof(pthread_t) * maxThreadNum);

		// 初始化线程池中的几个变量
		pool->minNum = minThreadNum;
		pool->maxNum = maxThreadNum;
		pool->livingNum = minThreadNum;
		pool->workingNum = 0;
		pool->exitNum = 0;
		pool->shutdown = 0;

		// 初始化4把锁，如果失败则退出
		if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
			pthread_mutex_init(&pool->mutexWorkingNum, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0 ||
			pthread_cond_init(&pool->notEmpty, NULL) != 0) {
			printf("mutex or cond init failed...\n");
			break;
		}

		// 初始化任务队列	
		pool->taskQueue = (Task*)malloc(sizeof(Task) * (maxCapacity + 1));
		pool->queueCapacity = maxCapacity;
		pool->queueSize = 0;
		pool->queueHead = 0;
		pool->queueTail = 0;

		// 创建管理者线程和最小数目的工作线程
		pthread_create(&pool->managerID, NULL, manage, pool);
		for (int i = 0; i < minThreadNum; i ++ )
			pthread_create(&pool->workersID[i], NULL, work, pool);
		
		return pool;
	} while (0);

	// malloc失败，将已申请的堆内存释放
	if (pool && pool->workersID) free(pool->workersID);
	if (pool && pool->taskQueue) free(pool->taskQueue);
	if (pool) free(pool);
	return NULL;
}

// 工作者线程添加任务
void threadPoolAdd(ThreadPool* pool, Task* task) {
	pthread_mutex_lock(&pool->mutexPool);
	while (pool->queueSize == pool->queueCapacity && !pool->shutdown)
		pthread_cond_wait(&pool->notFull, &pool->mutexPool);
	if (pool->shutdown) {
		pthread_mutex_unlock(&pool->mutexPool);
		return;
	}
	// 添加任务
	pool->taskQueue[pool->queueTail].func = task->func;
	pool->taskQueue[pool->queueTail].arg = task->arg;

	// 队尾向后移动
	pool->queueTail = (pool->queueTail + 1) % pool->queueCapacity;

	pool->queueSize ++ ;

	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->mutexPool);
}

// 工作者线程执行任务
void* work(void* arg) {
	ThreadPool* pool = (ThreadPool*)arg;

	while (1) {
		pthread_mutex_lock(&pool->mutexPool);
		// 如果任务队列为空，阻塞work线程
		while (!pool->shutdown && pool->queueSize == 0) {
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);  // 条件变量和互斥锁搭配使用
			if (pool->exitNum > 0) {  // 管理者线程判断进程数目过多，需要有退出
				pool->exitNum -- ;
				if (pool->livingNum > pool->minNum){
					pool->livingNum -- ;
					pthread_mutex_unlock(&pool->mutexPool);
					threadExit(pool);
				}
			}
		}

		if (pool->shutdown) {
			pthread_mutex_unlock(&pool->mutexPool);
			threadExit(pool);
		}
		
		// 从任务队列中取一个任务
		Task task;
		task.func = pool->taskQueue[pool->queueHead].func;
		task.arg = pool->taskQueue[pool->queueHead].arg;
		pool->queueHead = (pool->queueHead + 1) % pool->queueCapacity;  // 队头后移，使用循环队列
		pool->queueSize -- ;

		pthread_cond_signal(&pool->notFull);
		pthread_mutex_unlock(&pool->mutexPool);

		// 修改临界资源需加锁
		pthread_mutex_lock(&pool->mutexWorkingNum);
		pool->workingNum ++ ;
		pthread_mutex_unlock(&pool->mutexWorkingNum);
		
		// 执行一个任务
		printf("Thread %lu: start working...\n", pthread_self());
		task.func(task.arg);
		free(task.arg);
		task.arg = NULL;
		printf("Thread %lu: end working...\n", pthread_self());

		// 修改临界资源需加锁
		pthread_mutex_lock(&pool->mutexWorkingNum);
		pool->workingNum -- ;
		pthread_mutex_unlock(&pool->mutexWorkingNum);
	}
	return NULL;
}

// 管理者线程的管理函数
void* manage(void* arg) {
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown) {
		// 每隔3秒监测一次
		sleep(3);

		// 加锁并访问池中的几个参数
		pthread_mutex_lock(&pool->mutexPool);
		int taskNum = pool->queueSize;
		int livingNum = pool->livingNum;
		pthread_mutex_unlock(&pool->mutexPool);

		pthread_mutex_lock(&pool->mutexWorkingNum);
		int workingNum = pool->workingNum;
		pthread_mutex_unlock(&pool->mutexWorkingNum);

		// 线程不够，添加线程
		if (taskNum > livingNum && livingNum < pool->maxNum) {
			pthread_mutex_lock(&pool->mutexPool);
			for (int i = pool->minNum, cnt = 0; i < pool->maxNum; i ++ )
				if (pool->workersID[i] == 0 && cnt < NUM && livingNum < pool->maxNum) {
					pthread_create(&pool->workersID[i], NULL, work, pool);
					cnt ++ ;
					pool->livingNum ++ ;
				}
			pthread_mutex_unlock(&pool->mutexPool);
		}

		// 工作的线程 * 2 < 存活的线程 && 存活的线程 < 最小线程数
		// 线程过多，销毁线程
		if (workingNum * 2 < livingNum && livingNum > pool->minNum) {
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUM;  // 需要有线程退出
			pthread_mutex_unlock(&pool->mutexPool);
			for (int i = 0; i < NUM; i ++ )  
				pthread_cond_signal(&pool->notEmpty);
		}
	}
	return NULL;
}

int threadPoolWorkingNum(ThreadPool* pool) {
	pthread_mutex_lock(&pool->mutexWorkingNum);
	int num = pool->workingNum;
	pthread_mutex_unlock(&pool->mutexWorkingNum);

	return num;
}

int threadPoolLivingNum(ThreadPool* pool) {
	pthread_mutex_lock(&pool->mutexPool);
	int num = pool->livingNum;
	pthread_mutex_unlock(&pool->mutexPool);

	return num;
}

// 除了要将线程退出，还要将对应的存储的线程ID清零
void threadExit(ThreadPool* pool) {
	pthread_t tid = pthread_self();
	for (int i = 0; i < pool->maxNum; i ++ ) {
		if (pool->workersID[i] == tid) {
			pool->workersID[i] = 0;
			printf("thread %lu exiting...\n", tid);
			break;
		}
	}
	pthread_exit(NULL);
}

// 线程池销毁函数
int threadPoolDestroy(ThreadPool* pool) {
	if (!pool) return -1;

	pool->shutdown = 1;

	// 阻塞回收管理者线程
	pthread_join(pool->managerID, NULL);

	// 唤醒阻塞的工作者线程
	for (int i = 0; i < pool->livingNum; i ++ )
		pthread_cond_signal(&pool->notEmpty);

	// 释放堆内存
	if (pool->taskQueue) {
		free(pool->taskQueue);
		pool->taskQueue = NULL;
	}
	if (pool->workersID) {
		free(pool->workersID);
		pool->workersID = NULL;
	}
	// printf("asdf\n");
	pthread_mutex_destroy(&pool->mutexPool);

	pthread_mutex_destroy(&pool->mutexWorkingNum);
	// printf("adsfasdf\n");
	// pthread_cond_destroy(&pool->notEmpty);
	// printf("asdfadsf\n");
	pthread_cond_destroy(&pool->notFull);
	// printf("asdfadsf\n");

	// printf("asdf\n");
	free(pool);
	pool = NULL;
	return 0;
}
