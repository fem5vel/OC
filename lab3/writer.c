#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "semaphore.h"
#include "writer.h"

void writer_process(int semid, char *shared_memory)
{
    FILE *output_file = fopen("output.txt", "w");
    if (!output_file)
    {
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        semaphore_wait(semid, COUNTING_SEM);

        if (shared_memory[0] == '\0')
        {
            break;
        }

        fprintf(output_file, "%s", shared_memory);
        printf("Writer process: Wrote data from shared memory to output file.\n");

        semaphore_signal(semid, MUTEX_SEM);
    }

    fclose(output_file);
    exit(EXIT_SUCCESS);
}
