threadPool:main.o threadpool.o
	gcc -o threadPool main.o threadpool.o -lpthread

main.o:main.c
	gcc -c main.c

threadpool.o:threadpool.c
	gcc -c threadpool.c

.PHONY:clean
clean:
	rm threadPool *.o
