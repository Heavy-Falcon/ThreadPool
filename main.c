#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "threadpool.h"
#include <stdlib.h>

void* taskFunc(void* arg) {
	int n = *(int*)arg;
	printf("n = %d, tid = %lu\n", n, pthread_self());
	sleep(1);
	return NULL;
}

int main() {
	printf("请分别输入：\n任务队列的最大容量：");
	int maxCapacity, maxThreadNum, minThreadNum;
	scanf("%d", &maxCapacity);
	printf("最大线程数：");
	scanf("%d", &maxThreadNum);
	printf("最小线程数：");
	scanf("%d", &minThreadNum);
	ThreadPool* pool = threadPoolInit(maxCapacity, maxThreadNum, minThreadNum);
	Task task = {taskFunc, NULL};
	for (int i = 0; i < 100; i ++ ) {
		int* n = (int*)malloc(sizeof(int));
		*n = i;
		task.arg = n;
		threadPoolAdd(pool, &task);
	}
	
	sleep(22);

	threadPoolDestroy(pool);
	return 0;
}
