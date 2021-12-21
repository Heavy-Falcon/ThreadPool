#include "threadpool.hpp"

void* taskFunc(void* arg) {
	int n = *(int*)arg;
	cout << "Task No." << n << endl;
	sleep(1);
	return nullptr;
}

int main() {
	int maxCapacity, maxThreadNum, minThreadNum;
	
	cout << "请分别输入：" << endl;
	
	cout << "任务队列的最大容量：";
	cin >> maxCapacity;
	cout << "最大线程数：";
	cin >> maxThreadNum;
	cout << "最小线程数：";
	cin >> minThreadNum;

	// 创建线程池
	ThreadPool pool = ThreadPool(maxCapacity, maxThreadNum, minThreadNum);
	
	// 往任务队列中添加任务
	Task task = Task(taskFunc);
	for (int i = 1; i <= 100; i ++ ) {
		int* n = new int(i);
		*n = i;
		task.arg = n;
		pool.add(task);
	}
	
	sleep(22);

	return 0;
}
