/* $begin tinymain */
/* concurrent tiny HTTP server based on pre threaded
/* each thread accept by it self
 */

#include "do_task.h"

void* thread(void *vargp);
/*global varabiles to socket*/
int listenfd;
pthread_mutex_t thead_lock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {
  int port;
 
  pthread_t tid;
  int NTHREADS,SBUFSIZE;

  /* Check command line args */
  if (argc != 3) {
    fprintf(stderr, "usage: %s <port> <num/thread> \n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);
  NTHREADS = atoi(argv[2]);

  Signal(SIGPIPE,SIG_IGN);
  Signal(SIGCHLD,sigchld_hander);

  listenfd = Open_listenfd(port);

  int i;
  for(i=0;i<NTHREADS;i++)/*create worker threads*/
    Pthread_create(&tid,NULL,thread,NULL);

  while (1) {
    //connfd = Malloc(sizeof(int));//avoid race condition
    pause();
   
  }
}
/* $end tinymain */  
void *thread(void *vargp)
{
  Pthread_detach(pthread_self());
  int connfd;
  struct sockaddr_in clientaddr;
  int clientlen = sizeof(clientaddr);

  while(1)
  {
    Pthread_mutex_lock(&thead_lock);
    connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);
    Pthread_mutex_unlock(&thead_lock);
    doit(connfd);
    Close(connfd);
  }
}