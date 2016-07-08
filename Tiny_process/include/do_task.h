#ifndef __DOTASK_H__
#define __DOTASK_H__

#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize,int is_head);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs,int is_head);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void sigchld_hander(int sig);
void sigpipe_hander(int sig);
ssize_t post_read_requesthdrs(rio_t *rp);

void parse_post_request(rio_t* rio,char *post_data,char *uri,char *filename);

#endif

