#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "shared_memory.h"

int create_shared_memory()
{
    int shm_fd = shm_open("/shm_segment", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }
    ftruncate(shm_fd, SHM_SIZE);
    return shm_fd;
}

char *map_shared_memory(int shm_fd)
{
    char *shared_memory = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED)
    {
        perror("Failed to map shared memory");
        exit(EXIT_FAILURE);
    }
    return shared_memory;
}

void cleanup_shared_memory(char *shared_memory, int shm_fd)
{
    munmap(shared_memory, SHM_SIZE);
    close(shm_fd);
    shm_unlink("/shm_segment");
}
