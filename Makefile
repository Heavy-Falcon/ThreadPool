threadPool:main.o threadpool.o
	g++ -o threadPool main.o threadpool.o -lpthread

main.o:main.cpp
	g++ -c main.cpp

threadpool.o:threadpool.cpp
	g++ -c threadpool.cpp

.PHONY:clean
clean:
	rm threadPool *.o
