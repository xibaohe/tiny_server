/* $begin tinymain */
/* concurrent tiny HTTP server based on pre threaded
/* the buffer used condition varible
 */

#include "do_task.h"
#include "fdbuf_mutex.h"

void* thread(void *vargp);

sbuf_t sbuf; /*global shared buffer of connected fd*/

int main(int argc, char **argv) {
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;
  clientlen = sizeof(clientaddr);
  pthread_t tid;
  int NTHREADS,SBUFSIZE;

  /* Check command line args */
  if (argc != 4) {
    fprintf(stderr, "usage: %s <port> <num/thread> <fdbufsize>\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);
  NTHREADS = atoi(argv[2]);
  SBUFSIZE = atoi(argv[3]);

  Signal(SIGPIPE,SIG_IGN);
  Signal(SIGCHLD,sigchld_hander);

  sbuf_init(&sbuf,SBUFSIZE);
  listenfd = Open_listenfd(port);

  int i;
  for(i=0;i<NTHREADS;i++)/*create worker threads*/
    Pthread_create(&tid,NULL,thread,NULL);

  while (1) {
    //connfd = Malloc(sizeof(int));//avoid race condition
    connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);
    sbuf_insert(&sbuf,connfd);     
  }
}
/* $end tinymain */  
void *thread(void *vargp)
{
  Pthread_detach(pthread_self());
  while(1)
  {
    int connfd = sbuf_remove(&sbuf);
    doit(connfd);
    Close(connfd);
  }
}