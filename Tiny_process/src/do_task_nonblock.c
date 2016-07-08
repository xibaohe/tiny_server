#include "do_task_nonblock.h"

extern int verbose;

void sigchld_hander(int sig) {
  while (waitpid(-1, 0, WNOHANG) > 0) {
    if(verbose)
      printf("a child process gone!!\n");
  }
  return;
}
/* used to make sfd non-blocking */
int make_socket_non_blocking(int sfd) {
  int flags, s;
  flags = fcntl(sfd, F_GETFL, 0); /*the thrid parameter is ignored*/
  if (flags == -1) {
    unix_error("fcntl error!");
    return -1;
  }

  flags |= O_NONBLOCK;
  s = fcntl(sfd, F_SETFL, flags);
  if (s == -1) {
    unix_error("fcntl error");
    return -1;
  }
  return 0;
}

/**
 * to init the request_b struct
 * fd: the fd for connection
 * eptd: the fd of epoll
 */
void request_init(request_b *rb, int fd, int epfd) {
  rb->fd = fd;
  rb->epfd = epfd;
  rb->pos = rb->last = 0;
}

void doit_nonblock(request_b *rb) {
  char *plast = NULL;
  size_t remain_size;
  int n;
  while (1) {
    plast = &rb->buf[rb->last % MAXBUF];
    remain_size = MAXBUF - rb->last % MAXBUF ;
    //remain_size = 3;
    n = read(rb->fd, plast, remain_size);

    if (n == 0) {
      if (verbose) printf("read return 0, close fd\n");
      goto close;
    }

    if (n < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        /* other errer */
        printf("read error  errno = %d\n", errno);
        goto close;
      }
      break;
    }

    rb->last += n;
    assert(rb->last - rb->pos < MAXBUF);

    if (verbose) printf("try to parse the head\n");

    if (verbose) printf("the request is \n%s\n", rb->buf);

    if (strcmp(&rb->buf[rb->last - 2], "\r\n") != 0) {
      printf("the requst is not complete \n");
      continue;
    }
  }

  parse_request(rb);

  /*if allows the long connection*/
  // struct epoll_event event;
  // event.data.ptr = rb;
  // event.events =  EPOLLIN | EPOLLET | EPOLLONESHOT;
  // Epoll_ctl(rb->epfd,EPOLL_CTL_MOD,rb->fd,&event);
  close_conn(rb);
  return;
close:
  close_conn(rb);
}

size_t getlinefrombuf(request_b *rb, char *line) {
  int n;
  char *bufp = line;
  for (n = rb->pos; n <= rb->last; n++) {
    *bufp++ = rb->buf[n];
    if (rb->buf[n] == '\n') break;
  }
  *bufp = 0;  // end of the line!!!!
  rb->pos = (n + 1);
  return n;
}

void parse_request(request_b *rb) {
  int is_static;
  struct stat sbuf;
  char line[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  int is_head = 0;

  if (verbose) {
    printf("the rb->buf start and end:%zu %zu\n", rb->pos, rb->last);
  }
  getlinefrombuf(rb, line);
  sscanf(line, "%s %s %s", method, uri, version);
  if (verbose) {
    printf("the firstline of request\n%s", line);
  }

  if (!strcasecmp(method, "GET")) {  //
    read_requesthdrs(rb);
    is_static = parse_uri(uri, filename, cgiargs);
  } else if (!strcasecmp(method, "HEAD")) {
    read_requesthdrs(rb);
    is_static = parse_uri(uri, filename, cgiargs);
    is_head = 1;
  } else if (!strcasecmp(method, "POST")) {
    parse_post_request(rb, cgiargs, uri, filename);
    is_static = 0;  // always dynamic follow by parse_uri
    printf("cgiargs parameter is %s\n", cgiargs);
  } else {
    clienterror(rb->fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
    return;
  }

  if (stat(filename, &sbuf) < 0) {
    clienterror(rb->fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
    return;
  }

  if (is_static) { /* Serve static content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(rb->fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
      return;
    }
    serve_static(rb->fd, filename, sbuf.st_size, is_head);
  } else { /* Serve dynamic content */
    if (!(S_ISREG(sbuf.st_mode)) ||
        !(S_IXUSR & sbuf.st_mode)) {  // line:netp:doit:executable
      clienterror(rb->fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(rb->fd, filename, cgiargs,
                  is_head);  // line:netp:doit:servedynamic
  }
}

void close_conn(request_b *rb) {
  Close(rb->fd);
  free(rb);
}

/*
to parse the post request
*/
void parse_post_request(request_b *rb, char *post_data, char *uri,
                        char *filename) {
  char temp[MAXLINE];
  ssize_t content_lenth = post_read_requesthdrs(rb);
  if (rb->last - rb->pos != content_lenth) {
   
    unix_error("read post data error!!");
  }
  if(verbose)
  {
  	 printf("content_lenth is  %zu\n", content_lenth);
  }
  size_t n,i;
  for (n = rb->pos,i=0; n <= rb->last; n++,i++) 
  	temp[i] = rb->buf[n];
  //already contains null terminate
  //*(temp + content_lenth) = '\0'; /*to set the terminater!!!!!!!*/
  if (verbose) printf("Post data is %s\n", temp);
  /**/
  char *ptr1 = index(temp, '=');
  char *ptr2 = index(temp, '&');
  char *ptr3 = index(ptr2, '=');

  strncpy(post_data, ptr1 + 1, ptr2 - ptr1);
  strcat(post_data, ptr3 + 1);

  strcpy(filename, ".");
  strcat(filename, uri);
}

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(request_b *rb) {
  char line[MAXLINE];
  getlinefrombuf(rb, line);
  if (verbose) printf("read GET http requst header\n%s", line);
  while (strcmp(line, "\r\n")) {
    getlinefrombuf(rb, line);
    if (verbose) printf("%s", line);
  }
  return;
}

ssize_t post_read_requesthdrs(request_b *rb) {
  char line[MAXLINE];
  ssize_t content_lenth;
  getlinefrombuf(rb, line);
  if (verbose) printf("read Post http requst header\n%s", line);
  while (strcmp(line, "\r\n")) {
    if (strstr(line, "Content-Length")) {  // to  get the post
      char s_num[256];
      char *ptr = index(line, ':');
      if (ptr) {
        strcpy(s_num, ptr + 2);
        content_lenth = atoi(s_num);
      }
    }
    getlinefrombuf(rb, line);
    if (verbose) printf("%s", line);
  }
  return content_lenth;
}

/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;

  if (!strstr(uri, "cgi-bin")) {
    /* Static content */
    strcpy(cgiargs, "");              //  clearcgi
    strcpy(filename, ".");            //  beginconvert1
    strcat(filename, uri);            //  endconvert1
    if (uri[strlen(uri) - 1] == '/')  //  slashcheck
      strcat(filename, "home.html");  //  appenddefault
    return 1;
  } else { /* Dynamic content */  //  isdynamic
    ptr = index(uri, '?');        //  beginextract
    if (ptr) {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    } else
      strcpy(cgiargs, "");  //
    strcpy(filename, ".");  //
    strcat(filename, uri);  //
    return 0;
  }
}
/* $end parse_uri */
/*
 * serve_static - copy a file back to the client
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize, int is_head) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);     // line:netp:servestatic:getfiletype
  sprintf(buf, "HTTP/1.0 200 OK\r\n");  // line:netp:servestatic:beginserve
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  // char c;
  // c = getchar();
  // printf("%c\n", c);/*to ignore the EPIPE error!!! */
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));  // line:netp:servestatic:endserve

  if (is_head) {
    return;  // the request is head
  }

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);  // line:netp:servestatic:open
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);  //
  Close(srcfd);                                                //
  Rio_writen(fd, srcp, filesize);                              //
  Munmap(srcp, filesize);                                      //
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype) {
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head) {
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));
  if (is_head) return;  // the request is head

  if (Fork() == 0) { /* child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    /* Redirect stdout to client */
    Execve(filename, emptylist, environ);
    /* Run CGI program */
  }
  // Wait(NULL);
  /* Parent waits for and reaps child */
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body,
          "%s<body bgcolor="
          "ffffff"
          ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */
