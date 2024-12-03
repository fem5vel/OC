#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "semaphore.h"
#include "shared_memory.h"
#include "reader.h"
#include "writer.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <input_file> <segment_size>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *input_filename = argv[1];
    size_t segment_size = atoi(argv[2]);

    int shm_fd = create_shared_memory();
    char *shared_memory = map_shared_memory(shm_fd);
    int semid = create_semaphores();

    init_semaphore(semid, MUTEX_SEM, 0);
    init_semaphore(semid, COUNTING_SEM, 0);

    pid_t reader_pid = fork();
    if (reader_pid == 0)
    {
        reader_process(semid, shared_memory, input_filename, segment_size);
    }

    pid_t writer_pid = fork();
    if (writer_pid == 0)
    {
        writer_process(semid, shared_memory);
    }

    waitpid(reader_pid, NULL, 0);
    semaphore_signal(semid, COUNTING_SEM);
    waitpid(writer_pid, NULL, 0);

    cleanup_shared_memory(shared_memory, shm_fd);
    cleanup_semaphores(semid);

    return 0;
}
