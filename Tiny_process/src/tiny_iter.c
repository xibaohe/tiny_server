/*
*  iter HTTP Server
*/

#include "do_task.h"

int main(int argc,char **argv)
{
	int listenfd,connfd,port,clientlen;
	struct sockaddr_in clientaddr;
	clientlen = sizeof(clientaddr);

	if(argc != 2)
	{
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);
	Signal(SIGCHLD,sigchld_hander);
	listenfd = Open_listenfd(port);

	while(1)
	{
		connfd = Accept(listenfd,(SA *)&clientaddr,&clientlen);
		doit(connfd);
		Close(connfd); 
	}
}

