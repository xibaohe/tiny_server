/*
*  concurrent HTTP server based on select
*/

#include "do_task.h"

typedef struct 
{
	int maxfd;
	fd_set read_set; 
	fd_set ready_set;
	int nready;
	int maxi;
	int clientfd[FD_SETSIZE];
} pool;

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
	Signal(SIGCHLD,sigchld_hander);
	listenfd = Open_listenfd(port);
	init_pool(listenfd,&client_pool);

	sigset_t sig_chld;
	sigemptyset(&sig_chld);
	sigaddset(&sig_chld,SIGCHLD);

	while(1)
	{
		client_pool.ready_set = client_pool.read_set;
		while((client_pool.nready = Select(client_pool.maxfd+1,&client_pool.ready_set,NULL,NULL,NULL)) < 0)
		{
			if(errno == EINTR)
				printf("got a signal restart pselect!!! \n");
		}
		/*mask SIGCHLD!!!!!    but some signal will be abondoned
		*/
		//client_pool.nready = Pselect(client_pool.maxfd+1,&client_pool.ready_set,NULL,NULL,NULL,&sig_chld);
		if(FD_ISSET(listenfd,&client_pool.ready_set)){
			connfd = Accept(listenfd,(SA *)&clientaddr,&clientlen);
			add_client(connfd,&client_pool);
		} 
		check_clients(&client_pool);
	}
}
void init_pool(int listenfd,pool *p)
{
	int i;
	p->maxi = -1;
	for(i=0;i<FD_SETSIZE;i++)
		p->clientfd[i] = -1;

	p->maxfd = listenfd;
	FD_ZERO(&(p->read_set));
	FD_SET(listenfd,&p->read_set);
}

void add_client(int connfd,pool *p)
{
	int i;
	p->nready--;
	for(i =0;i< FD_SETSIZE;i++)
	{
		if(p->clientfd[i] < 0)
		{
			p->clientfd[i] = connfd;
			FD_SET(connfd,&p->read_set);

			if(connfd > p->maxfd)
				p->maxfd = connfd;
			if(i > p->maxi)
				p->maxi = i;
			break;
		}
	}
	if(i == FD_SETSIZE)
		app_error("add_client error: Too many clients");
}

void check_clients(pool *p)
{
	int i,connfd,n;

	for(i=0;(i<=p->maxi) && (p->nready > 0);i++)
	{
		connfd = p->clientfd[i];
		if((connfd > 0) && (FD_ISSET(connfd,&p->ready_set))){
			p->nready--;
			doit(connfd);
			Close(connfd); 
			FD_CLR(connfd, &p->read_set);
			p->clientfd[i] = -1;
		}

	}
}
