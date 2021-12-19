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
	ThreadPool* pool = threadPoolInit(20, 10, 3);
	Task *task;
	task->func = taskFunc;
	for (int i = 0; i < 100; i ++ ) {
		int* n = (int*)malloc(sizeof(int));
		*n = i;
		task->arg = n;
		threadPoolAdd(pool, task);
	}
	
	sleep(25);

	threadPoolDestroy(pool);
	return 0;
}
