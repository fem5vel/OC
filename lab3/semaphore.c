#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include "semaphore.h"

int create_semaphores()
{
    int semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("Failed to create semaphore set");
        exit(EXIT_FAILURE);
    }
    return semid;
}

void init_semaphore(int semid, int semnum, int init_val)
{
    union semun arg;
    arg.val = init_val;
    if (semctl(semid, semnum, SETVAL, arg) == -1)
    {
        perror("semctl SETVAL failed");
        exit(EXIT_FAILURE);
    }
}

void semaphore_wait(int semid, int semnum)
{
    struct sembuf sop;
    sop.sem_num = semnum;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    if (semop(semid, &sop, 1) == -1)
    {
        perror("semop wait failed");
        exit(EXIT_FAILURE);
    }
}

void semaphore_signal(int semid, int semnum)
{
    struct sembuf sop;
    sop.sem_num = semnum;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    if (semop(semid, &sop, 1) == -1)
    {
        perror("semop signal failed");
        exit(EXIT_FAILURE);
    }
}

void cleanup_semaphores(int semid)
{
    semctl(semid, 0, IPC_RMID);
}
