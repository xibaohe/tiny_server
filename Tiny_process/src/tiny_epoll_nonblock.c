/*
*  concurrent HTTP server based on epoll
*  nonblocking io
*/

#include "do_task_nonblock.h"
/*
typedef union epoll_data {
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event {
  uint32_t events;   // Epoll events
  epoll_data_t data; // User data variable
};
*/
#define MAXEVENTS 1024

int verbose = 0;

int main(int argc, char **argv) {
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;
  clientlen = sizeof(clientaddr);
  struct epoll_event *events; /* the ready events return from epoll_wait */

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  port = atoi(argv[1]);
  Signal(SIGCHLD, sigchld_hander);
  listenfd = Open_listenfd(port);

  make_socket_non_blocking(listenfd);
  int epfd = Epoll_create1(0);
  events =
      (struct epoll_event *)Malloc(sizeof(struct epoll_event *) * MAXEVENTS);

  request_b *request = (request_b *)Malloc(sizeof(request_b));
  request_init(request, listenfd, epfd);  //

  struct epoll_event event;  // event to register
  event.data.ptr = (void *)request;
  event.events = EPOLLIN | EPOLLET;
  Epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);

  sigset_t sig_chld;
  sigemptyset(&sig_chld);
  sigaddset(&sig_chld, SIGCHLD);

  while (1) {
    // int n = Epoll_wait(epfd,events,MAXEVENTS,-1); /* allways wait */
    // int n = Epoll_pwait(epfd,events,MAXEVENTS,-1,&sig_chld);
    int n;
    while ((n = Epoll_wait(epfd, events, MAXEVENTS, -1)) < 0) {
      if (errno == EINTR) printf("got a signal restart\n");
    }

    for (int i = 0; i < n; i++) {
      request_b *rb = (request_b *)events[i].data.ptr;
      // int fd = rb->fd;
      if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) ||
          (!(events[i].events & EPOLLIN))) {
        fprintf(stderr, "epoll error fd %d", rb->fd);
        Close(rb->fd);
        continue;
      } else if (rb->fd == listenfd) {
        /* the new connection incoming */
        int infd;
        while (1) {
          infd = accept(listenfd, (SA *)&clientaddr, &clientlen);
          if (infd < 0) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
              /*we have processed all incoming connections*/
              break;
            } else {
              unix_error("accept error!");
              break;
            }
          }

          make_socket_non_blocking(infd);
          if (verbose) printf("the new connection fd :%d\n", infd);
          request_b *request = (request_b *)Malloc(sizeof(request_b));
          request_init(request, infd, epfd);
          event.data.ptr = (void *)request;
          event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
          Epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &event);
        }
      }

      else {
        if (verbose) printf("new data from fd %d\n", rb->fd);

        doit_nonblock((request_b *)events[i].data.ptr);

        /* where to close the fd and release the requst!!! */
      }
    }
  }
  return 0;
}
