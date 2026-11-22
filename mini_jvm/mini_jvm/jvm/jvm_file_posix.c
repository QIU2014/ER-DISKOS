#include "jvm_file.h"

FILE *jvm_file_open(const char *path, const char *mode) {
    return fopen(path, mode);
}

size_t jvm_file_read(void *buffer, size_t size, size_t count, FILE *file) {
    return fread(buffer, size, count, file);
}

int jvm_file_seek(FILE *file, long offset, int origin) {
    return fseek(file, offset, origin);
}

long jvm_file_tell(FILE *file) {
    return ftell(file);
}

int jvm_file_eof(FILE *file) {
    return feof(file);
}

int jvm_file_close(FILE *file) {
    return fclose(file);
}
