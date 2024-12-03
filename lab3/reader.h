#ifndef READER_H
#define READER_H

void reader_process(int semid, char *shared_memory, const char *input_filename, size_t segment_size);

#endif
