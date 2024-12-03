#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "semaphore.h"
#include "reader.h"

void reader_process(int semid, char *shared_memory, const char *input_filename, size_t segment_size)
{
    FILE *input_file = fopen(input_filename, "r");
    if (!input_file)
    {
        perror("Failed to open input file");
        exit(EXIT_FAILURE);
    }

    size_t bytes_read;
    size_t memory_size = segment_size;

    while (1)
    {
        size_t read_size = memory_size - 1; 
        bytes_read = fread(shared_memory, 1, read_size, input_file);

        if (bytes_read == 0) 
            break;

        shared_memory[bytes_read] = '\0';
        printf("Reader process: Read %zu bytes into shared memory.\n", bytes_read);

        semaphore_signal(semid, COUNTING_SEM);
        semaphore_wait(semid, MUTEX_SEM);
    }

    shared_memory[0] = '\0';
    semaphore_signal(semid, COUNTING_SEM);
    fclose(input_file);
    exit(EXIT_SUCCESS);
}
