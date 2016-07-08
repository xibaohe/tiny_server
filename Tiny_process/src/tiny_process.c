/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 */

#include "do_task.h"

int main(int argc, char **argv) {
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;

  clientlen = sizeof(clientaddr);

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
    connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);
    if(Fork() == 0){//the children process
      Close(listenfd);
      doit(connfd);
      Close(connfd);
      exit(0);
    }
    Close(connfd);               
  }
}
/* $end tinymain */
