

#include "do_task.h"

int verbose = 0;

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) {
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;
  int is_head = 0;
  
  
  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version); // 
  if(verbose)
    printf("the firstline of request\n%s",buf);   
  
  if (!strcasecmp(method, "GET")) 
  { //
     read_requesthdrs(&rio); 
     is_static = parse_uri(uri, filename, cgiargs);
  }
  else if(!strcasecmp(method, "HEAD"))
  {
     read_requesthdrs(&rio);
     is_static = parse_uri(uri, filename, cgiargs);
     is_head = 1; 
  }
  else if(!strcasecmp(method,"POST"))
  {
    parse_post_request(&rio,cgiargs,uri,filename);
    is_static = 0;//always dynamic follow by parse_uri
    printf("cgiargs parameter is %s\n",cgiargs);
  }
  else{
     clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
    return;
  }
   
  if (stat(filename, &sbuf) < 0) {               
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
    return;
  }

  if (is_static) { /* Serve static content */
    if (!(S_ISREG(sbuf.st_mode)) ||
        !(S_IRUSR & sbuf.st_mode)) { 
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size,is_head); 
  } else {                                    /* Serve dynamic content */
    if (!(S_ISREG(sbuf.st_mode)) ||
        !(S_IXUSR & sbuf.st_mode)) { // line:netp:doit:executable
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs,is_head); // line:netp:doit:servedynamic
  }

  //while(1);
}
/* $end doit */
/*
to parse the post request
*/
void parse_post_request(rio_t* rio,char *post_data,char *uri,char *filename) {
  char temp[MAXLINE];
  ssize_t content_lenth = post_read_requesthdrs(rio);
  if (Rio_readnb(rio, temp, content_lenth) != content_lenth) {
    unix_error("read post data error!!");
  }
  *(temp + content_lenth) = '\0'; /*to set the terminater!!!!!!!*/
  /**/
  char* ptr1 = index(temp,'=');
  char* ptr2 = index(temp,'&');
  char* ptr3 = index(ptr2,'='); 
  
  strncpy(post_data,ptr1+1,ptr2-ptr1);
  strcat(post_data,ptr3+1);

  strcpy(filename, ".");  
  strcat(filename, uri); 

  // Rio_readlineb(&rio,param_buf,MAXLINE);
}

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  if(verbose)
    printf("read GET http requst header\n%s", buf);
  while (strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    if(verbose)
      printf("%s", buf);
  }
  return;
}

ssize_t post_read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  ssize_t content_lenth;
  Rio_readlineb(rp,buf,MAXLINE);
  if(verbose)
    printf("read Post http requst header\n%s", buf);
   while (strcmp(buf, "\r\n")) {
    
    if(strstr(buf,"Content-Length"))
    {//to  get the post 
      char s_num[256];
      char *ptr = index(buf,':');
      if(ptr)
      {
        strcpy(s_num,ptr+2);
        content_lenth = atoi(s_num);
      }
    }
    Rio_readlineb(rp, buf, MAXLINE);
    if(verbose)
      printf("%s", buf);
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
    strcpy(cgiargs, "");             //  clearcgi
    strcpy(filename, ".");           //  beginconvert1
    strcat(filename, uri);           //  endconvert1
    if (uri[strlen(uri) - 1] == '/') //  slashcheck
      strcat(filename, "home.html"); //  appenddefault
    return 1;
  } else { /* Dynamic content */ //  isdynamic
    ptr = index(uri, '?');       //  beginextract
    if (ptr) {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    } else
      strcpy(cgiargs, ""); // 
    strcpy(filename, "."); // 
    strcat(filename, uri); // 
    return 0;
  }
}
/* $end parse_uri */
/*
 * serve_static - copy a file back to the client
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize,int is_head) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);    // line:netp:servestatic:getfiletype
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // line:netp:servestatic:beginserve
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  // char c;
  // c = getchar();
  // printf("%c\n", c);/*to ignore the EPIPE error!!! */
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf)); // line:netp:servestatic:endserve


  if(is_head)
  {
    return;//the request is head
  }

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0); // line:netp:servestatic:open
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd,
              0);                 // 
  Close(srcfd);                   // 
  Rio_writen(fd, srcp, filesize); // 
  Munmap(srcp, filesize);         // 

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
  else if(strstr(filename,".mp4"))
    strcpy(filetype,"video/mp4");
  else
    strcpy(filetype, "text/plain");
}
/* $end serve_static */

void sigchld_hander(int sig)
{
  while(waitpid(-1,0,WNOHANG) > 0)
  {
    if(verbose)
      printf("a child process gone!!\n");
  }
  return;
}

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs,int is_head) {
  char buf[MAXLINE], *emptylist[] = {NULL};
 
 
  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));
  if(is_head)
    return ; //the request is head 

  if (Fork() == 0) { /* child */ 
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
        /* Redirect stdout to client */ 
    Execve(filename, emptylist, environ);
        /* Run CGI program */ 
  }
  //Wait(NULL);
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
  sprintf(body, "%s<body bgcolor="
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
