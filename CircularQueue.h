/*	
 *	CircularQueue.h
 *	
 *	(c) 2014 Eduardo Almeida and Joao Almeida
 *	SOPE 2013/2014 (T3G02)
 */

#include <semaphore.h>

typedef unsigned long QueueElem; 

typedef struct { 
	QueueElem *v;
	unsigned int first;
	unsigned int last;
	unsigned int capacity;
	sem_t empty;
	sem_t full;
	pthread_mutex_t mutex;		//	What for?
} CircularQueue;

void queue_init(CircularQueue **q, unsigned int capacity);

void queue_put(CircularQueue *q, QueueElem value);

QueueElem queue_get(CircularQueue *q); 

void queue_destroy(CircularQueue *q);