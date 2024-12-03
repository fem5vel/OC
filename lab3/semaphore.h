#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <sys/sem.h>

#define MUTEX_SEM 0
#define COUNTING_SEM 1

int create_semaphores();
void init_semaphore(int semid, int semnum, int init_val);
void semaphore_wait(int semid, int semnum);
void semaphore_signal(int semid, int semnum);
void cleanup_semaphores(int semid);

#endif
