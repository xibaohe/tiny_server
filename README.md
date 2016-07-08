# tiny_server
csapp tiny升级版

僅支持簡單GET、POST、HEAD方法



+ tiny_process:每个连接开一个进程
+ tiny_thread:每个连接开一个线程
+ tiny_thread_pre:事先创建线程池，由主线程同一accept，fdbuffer采用信号量同步(同csapp第12章)
+ tiny_thread_mutex: 同上，fdbuffer采用互斥锁和条件变量实现
+ tiny_thread_pre2: 事先创建线程池，每个线程各自accept。
+ tiny_select: select 阻塞I/O
+ tiny_poll: poll 阻塞I/O
+ tiny_epoll_nonblock 非阻塞I/O模式


