#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <iostream>
#include <cstring>
#include <pthread.h>
#include <vector>
#include <queue>
#include <unistd.h>

using namespace std;

// 定义任务结构体
class Task {
public:
	Task() {};
	Task(void* (*func)(void*)) {
		this->func = func;
		this->arg = NULL;
	}
	Task(void* (*func)(void*), void* arg) {
		this->func = func;
		this->arg = arg;
	}
	void* (*func)(void* arg);
	void* arg;
};

// 定义线程池类
class ThreadPool {
public:
	// 任务队列
	queue<Task> taskQueue;
	int queueCapacity;	// 队列中能容纳的最大任务数

	// 管理者线程
	pthread_t managerID;

	// 工作的线程
	vector<pthread_t> workersID;
	int minNum;		// 最小线程数
	int maxNum;		// 最大线程数
	int exitNum;	// 要销毁的线程

	pthread_mutex_t mutexPool;			// 锁整个线程池
	pthread_mutex_t mutexWorkingNum;	// 锁workingNum变量
	pthread_cond_t empty;	// 任务队列不满
	pthread_cond_t full;		// 任务队列不空

	bool shutdown;
	ThreadPool(int maxCapacity, int maxThreadNum, int minThreadNum);
	void exit();
	void add(Task* task);
	int getWorkingNum();
	int getLivingNum();
	void setWorkingNum(int newNum);
	void setLivingNum(int newNum);
	~ThreadPool();
private:
	int livingNum;	// 存活的线程
	int workingNum;	// 正在工作的线程
};

void* manage(void* arg);
void* work(void* arg);

#endif
