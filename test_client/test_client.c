#include "csapp.h"
#define BILLION 1E9

int main(int argc, char** argv) {
  // to do : to complete the communication between process
  int clientfd, port;
  char* host;
  char request_buf[MAXBUF], receive_buf[MAXBUF];
  int nchildren, nloops;  // the prosses and the num of request for each process
  rio_t rio;

  if (argc != 5) {
    fprintf(stderr, "usage: %s <host> <port> <#children> <#loops/child>\n",
            argv[0]);
    exit(0);
  }
  host = argv[1];
  port = atoi(argv[2]);
  nchildren = atoi(argv[3]);
  nloops = atoi(argv[4]);

  sprintf(request_buf, "HEAD / HTTP/1.1\r\n");
  sprintf(request_buf, "%sHOST: %s: %d\r\n\r\n", request_buf, host, port);

  int pipe[2];
  Pipe(pipe);

  int i, j, pid;
  double totaltime = 0;
  for (i = 0; i < nchildren; i++) {
    if ((pid = Fork()) == 0) {
      Close(pipe[0]);
      double child_total_time = 0;
      for (j = 0; j < nloops; j++) {
        struct timespec requestStart, requestEnd;
        clock_gettime(CLOCK_REALTIME, &requestStart);

        clientfd = Open_clientfd(host, port);
        Rio_readinitb(&rio, clientfd);
        Rio_writen(clientfd, request_buf, strlen(request_buf));
        Rio_readlineb(&rio, receive_buf, MAXLINE);
        Close(clientfd);

        clock_gettime(CLOCK_REALTIME, &requestEnd);
        double accum = (requestEnd.tv_sec - requestStart.tv_sec) +
                       (requestEnd.tv_nsec - requestStart.tv_nsec) / BILLION;
        child_total_time += accum;

        // printf("%s\n", receive_buf);
      }
      // printf("children %d id done\n", i);
      // printf( "the child_total_time is %lf\n", child_total_time );
      // printf( "the child_avg_time is %lf\n", child_total_time /
      // (double)nloops );
      /*the write to pipe is atomic!!!*/
      Write(pipe[1], &child_total_time, sizeof(child_total_time));
      exit(0);
    }
  }
  while (wait(NULL) > 0)
    ;
  Close(pipe[1]);
  double temp_time;
  while (Read(pipe[0], &temp_time, sizeof(temp_time)) > 0) {
    totaltime += temp_time;
  }
  printf("the total_time is %lf\n", totaltime);
  printf("the avg_time is %lf\n",
         totaltime / ((double)nloops * (double)nchildren));

  if (errno != ECHILD) app_error("wait error!!");

  exit(0);
}