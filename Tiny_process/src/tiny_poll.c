/*
*  concurrent HTTP server based on poll
*/

#include "do_task.h"
/*
struct pollfd{
	int fd; //fd to check
	short events; //events of interest on fd
	short revents; //events that occurred on fd
}
*/
typedef struct{
	struct pollfd client[OPEN_MAX];
	int maxi;
	int nready;
}pool;

void init_pool(int listenfd,pool *p);
void add_client(int connfd,pool *p);
void check_clients(pool *p);

int main(int argc,char **argv)
{
	int listenfd,connfd,port,clientlen;
	struct sockaddr_in clientaddr;
	clientlen = sizeof(clientaddr);
	static pool client_pool;

	if(argc != 2)
	{
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);
	Signal(SIGCHLD,sigchld_hander);// I/O multiplexing!!!!!  single process cannot handle this????why?
	listenfd = Open_listenfd(port);
	init_pool(listenfd,&client_pool);

	while(1)
	{
		while((client_pool.nready = Poll(client_pool.client,client_pool.maxi + 1,INFTIM)) < 0)
		{
			if(errno == EINTR)
				printf("got a signal restart poll!!! \n");
		}
		
		if(client_pool.client[0].revents & POLLRDNORM)
		{
			connfd = Accept(listenfd,(SA *)&clientaddr,&clientlen);
			add_client(connfd,&client_pool);
		}
		//if(p->nready <=0)continue;
		check_clients(&client_pool);
	}
}
void init_pool(int listenfd,pool *p)
{
	p->client[0].fd = listenfd;
	p->client[0].events = POLLRDNORM;
	int i;
	for(i = 1;i<OPEN_MAX;i++)
		p->client[i].fd = -1;
	p->maxi = 0;
}

void add_client(int connfd,pool *p)
{
	int i;
	p->nready--;

	for(i =1;i< OPEN_MAX;i++)
	{
		if(p->client[i].fd < 0)
		{
			p->client[i].fd = connfd;
			p->client[i].events = POLLRDNORM;
			if(i > p->maxi)
				p->maxi = i;
			break;
		}
	}
	if(i == OPEN_MAX)
		app_error("add_client error: Too many clients");
}

void check_clients(pool *p)
{
	int i,connfd,n;

	for(i=1;(i<=p->maxi) && (p->nready > 0);i++)
	{
		connfd = p->client[i].fd;
		if((connfd > 0) && (p->client[i].revents & (POLLRDNORM | POLLERR) )){
			p->nready--;
			doit(connfd);
			Close(connfd); 
			p->client[i].fd= -1;
		}

	}
}
