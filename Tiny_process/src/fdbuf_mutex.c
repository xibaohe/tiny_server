#include "fdbuf_mutex.h"


void sbuf_init(sbuf_t *sp, int n)
{
	sp->buf = Calloc(n,sizeof(int));
	sp->n = n;
	sp->front = sp->rear = 0;
	sp->nready = 0;
	sp->nslots = n;
	sp->buf_mutex = PTHREAD_MUTEX_INITIALIZER;
	sp->nready_mutex = PTHREAD_MUTEX_INITIALIZER;
	sp->cond = PTHREAD_COND_INITIALIZER;
}

void sbuf_deinit(sbuf_t *sp)
{
	Free(sp->buf);
}
void sbuf_insert(sbuf_t *sp, int item)
{
	/*write to the buffer*/
	Pthead_mutex_lock(&sp->buf_mutex);
	if(sp->nslots == 0)
	{
		Pthread_mutex_unlock(&sp->buf_mutex);
		return ;
	}
	sp->buf[(++sp->rear)%(sp->n)] = item;
	sp->nslots--;
	Pthread_mutex_unlock(&sp->buf_mutex);

	int dosignal = 0;
	Pthread_mutex_lock(&sp->nready_mutex);
	if(sp->nready == 0)
		dosignal = 1;
	sp->nready++;
	Pthread_mutex_unlock(&sp->nready_mutex)
	if(dosignal)
		Pthread_cond_signal(&sp->cond);
}
int sbuf_remove(sbuf_t *sp)
{
	int item;
	Pthread_mutex_lock(&sp->nready_mutex);
	while(sp->nready == 0)
		Pthread_cond_wait(&sp->cond,&sp->nready_mutex);
	item = sp->buf[(++sp->front) % (sp->n)];
	Pthread_mutex_unlock(&sp->nready_mutex);
	if(item == 0)fprintf(stderr, "error!!!!fd item%d\n", item);
	return item;
}
// void sbuf_insert(sbuf_t *sp, int item)
// {
// 	/*write to the buffer*/
// 	Pthead_mutex_lock(&sp->buf_mutex);
// 	if(sp->nslots == 0)
// 	{
// 		Pthread_mutex_unlock(&sp->buf_mutex);
// 		return ;
// 	}
// 	sp->buf[(++sp->rear)%(sp->n)] = item;
// 	sp->nslots--;
// 	int dosignal = 0;
// 	if(sp->nslots < sp->n)
// 		dosignal = 1;
// 	Pthread_mutex_unlock(&sp->buf_mutex);
// 	if(dosignal)
// 		Pthread_cond_signal(&sp->cond);
// }
// int sbuf_remove(sbuf_t *sp)
// {
// 	int item;
// 	Pthead_mutex_lock(&sp->buf_mutex);
// 	while(sp->nslots == n)
// 		Pthread_cond_wait(&sp->cond,&sp->buf_mutex);
// 	item = sp->buf[(++sp->front) % (sp->n)];
// 	Pthread_mutex_unlock(&sp->buf_mutex);
// 	if(item == 0)fprintf(stderr, "error!!!!fd item%d\n", item);
// 	return item;
// }