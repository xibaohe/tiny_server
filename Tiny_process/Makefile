CC=gcc
CFLAGS=-I./include -I../common -O2
LD=-lpthread
HPATH=../common
SRC=src
INCLUDE=include

all: tiny_iter tiny_process tiny_select tiny_poll tiny_thread tiny_thread_pre tiny_thread_pre_mutex tiny_thread_pre2 tiny_epoll_nonblock

tiny_epoll%: tiny_epoll%.o do_task_nonblock.o csapp.o 
	$(CC) -o $@ do_task_nonblock.o csapp.o $< $(LD)

tiny_%: tiny_%.o do_task.o csapp.o fdbuf.o
	$(CC) -o $@ do_task.o csapp.o fdbuf.o $< $(LD)

%.o: $(SRC)/%.c
	$(CC) -c $(CFLAGS)  $<
	
csapp.o: $(HPATH)/csapp.c
	$(CC) -c $(HPATH)/csapp.c $(CFLAGS)

clean:
	rm -rf tiny_*
	rm -rf *.o
send:
	scp -r * xibaohe@192.168.2.103:~/testServer
