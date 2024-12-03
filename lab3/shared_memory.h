#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#define SHM_SIZE 1024

int create_shared_memory();
char *map_shared_memory(int shm_fd);
void cleanup_shared_memory(char *shared_memory, int shm_fd);

#endif
