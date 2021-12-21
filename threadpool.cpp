#include "threadpool.hpp"
#define NUM 2

ThreadPool::ThreadPool(int maxCapacity, int maxThreadNum, int minThreadNum) {
	this->workersID = vector<pthread_t>(maxThreadNum, 0);

	// 初始化线程池中的几个变量
	minNum = minThreadNum;
	maxNum = maxThreadNum;
	livingNum = minThreadNum;
	queueCapacity = maxCapacity;
	workingNum = 0;
	exitNum = 0;
	shutdown = false;

	// 初始化4把锁，如果失败则退出
	pthread_mutex_init(&mutexPool, nullptr);
	pthread_mutex_init(&mutexWorkingNum, nullptr);
	// 队列为空时，工作者线程阻塞
	// 不空了则唤醒工作者线程
	// 队列满时，添加任务的函数阻塞
	// 不满了，则唤醒添加任务的函数
	pthread_cond_init(&full, nullptr);
	pthread_cond_init(&empty, nullptr);

	// 创建管理者线程和最小数目的工作线程
	pthread_create(&managerID, nullptr, manage, this);
	for (int i = 0; i < minThreadNum; i ++ )
		pthread_create(&workersID[i], nullptr, work, this);
	
}

// 工作者线程添加任务
void ThreadPool::add(Task& task) {
	pthread_mutex_lock(&mutexPool);
	// 当队列已满时，阻塞添加
	while (taskQueue.size() >= queueCapacity && !shutdown)
		pthread_cond_wait(&full, &mutexPool);
	if (shutdown) {
		pthread_mutex_unlock(&mutexPool);
		return;
	}
	// 添加任务
	taskQueue.push(task);

	// 有任务了，唤醒阻塞的工作者线程
	pthread_cond_signal(&empty);
	pthread_mutex_unlock(&mutexPool);
	// cout << "任务添加完成" << endl;
}

// 工作者线程执行任务
void* work(void* arg) {
	ThreadPool* pool = (ThreadPool*)arg;

	while (1) {
		pthread_mutex_lock(&pool->mutexPool);
		// 如果任务队列为空，阻塞工作者线程
		while (!pool->shutdown && pool->taskQueue.empty()) {
			pthread_cond_wait(&pool->empty, &pool->mutexPool);
			if (pool->exitNum > 0) {  // 管理者线程判断进程数目过多，需要有退出
				pool->exitNum -- ;
				pool->setLivingNum(pool->getLivingNum() - 1);
				pthread_mutex_unlock(&pool->mutexPool);
				pool->exit();
			}
		}

		if (pool->shutdown) {
			pthread_mutex_unlock(&pool->mutexPool);
			pool->exit();
		}
		
		// 从队头取一个任务
		Task task;
		task.func = pool->taskQueue.front().func;
		task.arg = pool->taskQueue.front().arg;
		pool->taskQueue.pop();

		// 取出任务后，任务队列不满了，唤醒阻塞的任务添加函数
		pthread_cond_signal(&pool->full);
		pthread_mutex_unlock(&pool->mutexPool);

		// 执行任务前，将忙碌的线程数加1
		// 修改临界资源需加锁
		pthread_mutex_lock(&pool->mutexWorkingNum);
		pool->setWorkingNum(pool->getWorkingNum() + 1);
		pthread_mutex_unlock(&pool->mutexWorkingNum);
		
		// 执行一个任务
		cout << "Thread " << pthread_self() << ": start working..." << endl;
		task.func(task.arg);
		cout << "Thread " << pthread_self() << ": end working..." << endl;

		// 任务执行完毕，忙碌线程数减1
		// 修改临界资源需加锁
		pthread_mutex_lock(&pool->mutexWorkingNum);
		pool->setWorkingNum(pool->getWorkingNum() - 1);
		pthread_mutex_unlock(&pool->mutexWorkingNum);
	}
	return nullptr;
}

// 管理者线程的管理函数
void* manage(void* arg) {
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown) {
		// 每隔3秒监测一次
		sleep(3);

		// 加锁并访问池中的几个参数
		pthread_mutex_lock(&pool->mutexPool);
		int taskNum = pool->taskQueue.size();  // 任务个数
		int livingNum = pool->getLivingNum();  // 活着的线程数
		pthread_mutex_unlock(&pool->mutexPool);

		pthread_mutex_lock(&pool->mutexWorkingNum);
		int workingNum = pool->getWorkingNum();
		pthread_mutex_unlock(&pool->mutexWorkingNum);

		// 线程不够，添加NUM个线程
		if (taskNum > livingNum && livingNum < pool->maxNum) {
			pthread_mutex_lock(&pool->mutexPool);
			for (int i = 0, cnt = 0; i < pool->maxNum && cnt < NUM && livingNum < pool->maxNum; i ++ ) {
				if (pool->workersID[i] == 0) {
					pthread_create(&pool->workersID[i], nullptr, work, pool);
					cnt ++ ;
					pool->setLivingNum(pool->getLivingNum() + 1);
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}

		// 工作的线程 * 2 < 存活的线程 && 存活的线程 > 最小线程数
		// 线程过多，销毁线程
		if (workingNum * 2 < livingNum && livingNum > pool->minNum) {

			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUM;  // 需要有NUM个线程退出
			pthread_mutex_unlock(&pool->mutexPool);

			for (int i = 0; i < NUM; i ++ )  
				pthread_cond_signal(&pool->empty);
		}
	}
	return nullptr;
}

int ThreadPool::getWorkingNum() {
	int num = workingNum;
	return num;
}

int ThreadPool::getLivingNum() {
	int num = livingNum;
	return num;
}

void ThreadPool::setWorkingNum(int n) {
	workingNum = n;
}

void ThreadPool::setLivingNum(int n) {
	livingNum = n;
}

// 将线程退出前，要先将对应的存储的线程ID清零
void ThreadPool::exit() {
	pthread_t tid = pthread_self();
	for (int i = 0; i < maxNum; i ++ ) {
		if (workersID[i] == tid) {
			workersID[i] = 0;
			cout << "thread " << tid << " exited" << endl;
			break;
		}
	}
	pthread_exit(nullptr);
}

// 线程池析构函数
ThreadPool::~ThreadPool() {
	shutdown = true;

	// 阻塞回收管理者线程
	pthread_join(managerID, nullptr);

	// 唤醒阻塞的工作者线程
	for (int i = 0; i < livingNum; i ++ )
		pthread_cond_signal(&empty);

	pthread_mutex_destroy(&mutexPool);
	pthread_mutex_destroy(&mutexWorkingNum);
	pthread_cond_destroy(&empty);
	pthread_cond_destroy(&full);
}
