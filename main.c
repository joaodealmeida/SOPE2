/*	
 *	main.c
 *	
 *	(c) 2014 Eduardo Almeida and Joao Almeida
 *	SOPE 2013/2014 (T3G02)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/mman.h>

#include "CircularQueue.h"

#define SHARED 0

#define NEXT_PRIME_POS_INDEX 0

#define DEBUG 0

char SHM1_NAME[] = "/SOPE_2_SHM1";

char SEM1_NAME[] = "/SOPE_2_SEM1";
char SEM2_NAME[] = "/SOPE_2_SEM2";

int _shared_region_size = 0;
int _max_number = 0;

/*
 *	Function that creates the circular queue with all the numbers from 2 to N.
 */

CircularQueue* create_all_numbers_queue(int max) {
	CircularQueue *queue;
	
	queue_init(&queue, max + 2);
	
	int i;
	
	for (i = 2; i <= max; i++)
		queue_put(queue, i);
	
	queue_put(queue, 0);
	
	return queue;
}

/*
 *	Function that gets the shared memory region.
 */

int get_shared_memory_region(unsigned int **array) {
	int shmfd = shm_open(SHM1_NAME, O_RDWR, 0600);

	if (shmfd < 0)
		return 1;

	if (ftruncate(shmfd, _shared_region_size) < 0)
		return 2;

	*array = (unsigned int *) mmap(NULL, _shared_region_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

	if (array == MAP_FAILED)
		return 3;

	return 0;
}

/*
 *	Function that creates the shared memory region.
 *	MUST be called before get_shared_memory_region().
 */

int create_shared_memory_region(int max) {
	_shared_region_size = ((1.2 * (max / log(max))) + 2) * sizeof(unsigned int);
	
	int shmfd = shm_open(SHM1_NAME, O_CREAT | O_RDWR, 0600);
	
	if (shmfd < 0)
		return 1;
	
	if (ftruncate(shmfd, _shared_region_size) < 0)
		return 2;
	
	unsigned int *array = mmap(NULL, _shared_region_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	
	if (array == MAP_FAILED)
		return 3;
	
	sem_t *semaphore = sem_open(SEM2_NAME, O_CREAT, 0600, NULL);
	
	if (semaphore == SEM_FAILED) {
		printf("Error while getting the Shared Memory semaphore.\n");
		
		exit(EXIT_FAILURE);
	}
	
	sem_post(semaphore);
	
	array[NEXT_PRIME_POS_INDEX] = 1;
	
	int i;
	
	for (i = 1; i < _shared_region_size + 2; i++)
		array[i] = 0;
	
	return 0;
}

/*
 *	Function that places a number into shared memory.
 */

int place_number_into_shared_memory(unsigned int number) {
	unsigned int *array;
	
	if (get_shared_memory_region(&array)) {
		printf("Error while getting the shared memory region.\n");
		
		return 1;
	}
	
	sem_t *semaphore = sem_open(SEM2_NAME, 0, 0600, NULL);
	
	sem_wait(semaphore);
	
	array[array[NEXT_PRIME_POS_INDEX]] = number;
	
	#if DEBUG
	printf("Placed Number %d on index %d...\n", array[array[NEXT_PRIME_POS_INDEX]], array[NEXT_PRIME_POS_INDEX]);
	#endif
	
	array[NEXT_PRIME_POS_INDEX]++;
	
	sem_post(semaphore);
	
	return 0;
}

/*
 *	Filter Thread.
 */

void *filter_thread(void *arg) {
	CircularQueue *entryQueue = (CircularQueue *) arg;
	
	CircularQueue *exitQueue;
	queue_init(&exitQueue, _max_number);
	
	unsigned int first = queue_get(entryQueue);
	
	if (place_number_into_shared_memory(first)) {
		printf("Error while accessing the shared memory region.\n");
		
		exit(EXIT_FAILURE);
	}
	
	#if DEBUG
	printf("Placed %d on the shared memory region.\n", first);
	#endif
	
	unsigned int number = 0;
	
	if (first > sqrt(_max_number)) {
		#if DEBUG
		printf("Finished, the rest are primes. (%d and up)\n", first);
		#endif
		
		while (( number = queue_get(entryQueue) ))	//	Get each number from the entry queue up until "0"
			place_number_into_shared_memory(number);
		
		//	Signal the EOP semaphore, we are done!
		
		sem_t *semaphore = sem_open(SEM1_NAME, 0, 0600, NULL);
		
		if (semaphore == SEM_FAILED) {
			printf("Error while getting the End of Program semaphore.\n");
			
			exit(EXIT_FAILURE);
		}
		
		sem_post(semaphore);
		
		return NULL;
	}
	
	pthread_t filter_tid;
	
	pthread_create(&filter_tid, NULL, filter_thread, exitQueue);
	
	while (( number = queue_get(entryQueue) ))	//	Get each number from the entry queue up until "0"
		if (number % first)	//	So far, the number is prime.
			queue_put(exitQueue, number);
	
	queue_put(exitQueue, 0);
	
	queue_destroy(entryQueue);
	
	return NULL;
}

/*
 *	Main Thread.
 */

void *main_thread(void *arg) {
	CircularQueue *entryQueue = create_all_numbers_queue(_max_number);
	
	CircularQueue *exitQueue;
	queue_init(&exitQueue, _max_number);
	
	pthread_t filter_tid;
	
	pthread_create(&filter_tid, NULL, filter_thread, exitQueue);
	
	unsigned int first = queue_get(entryQueue);
	
	if (place_number_into_shared_memory(first)) {
		printf("Error while accessing the shared memory region.\n");
		
		exit(EXIT_FAILURE);
	}
	
	#if DEBUG
	printf("Placed %d on the shared memory region.\n", first);
	#endif
	
	unsigned int number = 0;
	
	while (( number = queue_get(entryQueue) ))	//	Get each number from the entry queue up until "0"
		if (number % first)	//	Up so far, the number is not prime.
			queue_put(exitQueue, number);
	
	queue_put(exitQueue, 0);
	
	queue_destroy(entryQueue);
	
	return NULL;
}

/*
 *	Helper comparision function for use with qsort().
 */

int cq_compare(const void *a, const void *b) {
   return *(unsigned int *) a - *(unsigned int *) b;
}

/*
 *	The program entry point.
 */

int main(int argc, const char *argv[]) {
	int launchErr = 0;
	
	if (argc != 2)
		launchErr = 1;
	
	if (!launchErr)
		if ( !(_max_number = atoi(argv[1])) )
			launchErr = 1;
	
	if (launchErr) {
		char progname[512];
		strcpy(progname, argv[0]);
		
		printf("Usage: %s highest_number\n", basename(progname));
		
		return 0;
	}
	
	if (create_shared_memory_region(_max_number)) {
		printf("Error while creating the shared memory region.\n");
		
		return EXIT_FAILURE;
	}
	
	sem_t *semaphore = sem_open(SEM1_NAME, O_CREAT, 0600, NULL);
	
	if (semaphore == SEM_FAILED) {
		printf("An error has occoured while creating a semaphore.\n");
		
		return EXIT_FAILURE;
	}
	
	pthread_t main_tid;
	
	pthread_create(&main_tid, NULL, main_thread, NULL);
	
	sem_wait(semaphore);
	
	if (sem_close(semaphore))
		printf("An error has occoured while closing the semaphore. Continuing anyway...");
	
	unsigned int *sharedQueue;
	
	if (get_shared_memory_region(&sharedQueue)) {
		printf("Error while accessing the shared memory region.\n");
		
		exit(EXIT_FAILURE);
	}
	
	unsigned int count = sharedQueue[0];
	
	sharedQueue[0] = 0;
	
	qsort(sharedQueue, count, sizeof(unsigned int), cq_compare);
	 
	printf("Prime Numbers up to %d (%d): ", _max_number, count);
	
	unsigned int i;
	
	for (i = 1; i < count; i++)
		printf("%d ", sharedQueue[i]);
	
	printf("\n");
	
	if (shm_unlink(SHM1_NAME)) {
		printf("An error has occoured while closing the shared memory region.");
		
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
