#ifndef __DO_TASK_NONBLOCK__
#define __DO_TASK_NONBLOCK__ 

#include "csapp.h" 

typedef struct request_buffer{
	int fd;/*fd for the connection  */
	int epfd;/* fd for epoll */
	char buf[MAXBUF]; /*the buffer for the current request*/
	size_t pos,last;
}request_b;
#define MIN(a,b) ((a) < (b) ? (a) : (b))


void request_init(request_b *rb,int fd,int epfd);
void doit_nonblock(request_b *rb);
void close_conn(request_b * rb);
int make_socket_non_blocking(int sfd);
void sigchld_hander(int sig);
size_t getlinefrombuf(request_b *rb,char* line);

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head);


void get_filetype(char *filename, char *filetype);
void serve_static(int fd, char *filename, int filesize, int is_head);
int parse_uri(char *uri, char *filename, char *cgiargs);
ssize_t post_read_requesthdrs(request_b *rb);

void read_requesthdrs(request_b *rb);

void parse_post_request(request_b *rb, char *post_data, char *uri,
                        char *filename);
void close_conn(request_b *rb);
void parse_request(request_b *rb);
#endif