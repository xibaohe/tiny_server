/* $begin tinymain */
/* concurrent tiny HTTP server based on thread
 */

#include "do_task.h"

void* thread(void *vargp);

int main(int argc, char **argv) {
  int listenfd, *connfd, port, clientlen;
  struct sockaddr_in clientaddr;
  clientlen = sizeof(clientaddr);
  pthread_t tid;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);

  Signal(SIGPIPE,SIG_IGN);
  Signal(SIGCHLD,sigchld_hander);

  listenfd = Open_listenfd(port);
  while (1) {
    connfd = Malloc(sizeof(int));//avoid race condition
    *connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);
    Pthread_create(&tid,NULL,&thread,connfd);             
  }
}
/* $end tinymain */
void *thread(void *vargp)
{
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);
  doit(connfd);
  Close(connfd);
  return NULL;
}