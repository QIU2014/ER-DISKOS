#ifndef MINI_JVM_FILE_H
#define MINI_JVM_FILE_H

#include <stddef.h>
#include <stdio.h>

FILE *jvm_file_open(const char *path, const char *mode);
size_t jvm_file_read(void *buffer, size_t size, size_t count, FILE *file);
int jvm_file_seek(FILE *file, long offset, int origin);
long jvm_file_tell(FILE *file);
int jvm_file_eof(FILE *file);
int jvm_file_close(FILE *file);

#endif
