/*	
 *	CircularQueue.c
 *	
 *	(c) 2014 Eduardo Almeida and Joao Almeida
 * 	SOPE 2013/2014 (T3G02)
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "CircularQueue.h"

void _queue_rot(CircularQueue *q);	//	Prototype.

void queue_init(CircularQueue **q, unsigned int capacity) { 
	*q = (CircularQueue *) malloc(sizeof(CircularQueue)); 
	sem_init(&((*q)->empty), 0, capacity); 
	sem_init(&((*q)->full), 0, 0); 
	pthread_mutex_init(&((*q)->mutex), NULL); 
	(*q)->v = (QueueElem *) malloc(capacity * sizeof(QueueElem)); 
	(*q)->capacity = capacity;
	(*q)->first = 0; 
	(*q)->last = 0;
} 

void queue_put(CircularQueue *q, QueueElem value) {
	sem_wait(&(q->empty));	//	Proceeds if a value can be placed, blocks if not.
	
	pthread_mutex_lock(&(q->mutex));
	
	(q->v)[q->last] = value;
	
	q->last++;
	
	pthread_mutex_unlock(&(q->mutex));
	
	sem_post(&(q->full));
}

void _queue_rot(CircularQueue *q) {
	unsigned int i, j;
	
	for (i = 0, j = 1; i < q->last; i++, j++)
		(q->v)[i] = (q->v)[j];
	
	(q->v)[j] = 0;
}

QueueElem queue_get(CircularQueue *q) { 
	sem_wait(&(q->full));
	
	pthread_mutex_lock(&(q->mutex));
	
	QueueElem val = (q->v)[q->first];
	
	_queue_rot(q);
	
	q->last--;
	
	pthread_mutex_unlock(&(q->mutex));
	
	sem_post(&(q->empty));
	
	return val;
} 

void queue_destroy(CircularQueue *q) {
	//	Release the allocated objects...
	
	free(q->v);
	
	pthread_mutex_destroy(&(q->mutex));
	
	sem_destroy(&(q->empty));
	sem_destroy(&(q->full));
	
	//	Release the queue itself...
	
	free(q);	
}