#include "jvm_file.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "os/UEFIPlatform.h"

typedef struct {
    unsigned char *data;
    size_t size;
    size_t position;
} UefiMemoryFile;

FILE *jvm_file_open(const char *path, const char *mode) {
    size_t path_length = 0;
    CHAR16 *wide_path;
    VOID *data = NULL;
    UINTN size = 0;
    UefiMemoryFile *file;

    if (path == NULL || mode == NULL || mode[0] != 'r') {
        return NULL;
    }

    while (path[path_length] != '\0') {
        if ((unsigned char)path[path_length] > 0x7f) {
            return NULL;
        }
        path_length++;
    }

    wide_path = malloc((path_length + 1) * sizeof(CHAR16));
    if (wide_path == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < path_length; i++) {
        CHAR16 ch = (unsigned char)path[i];
        wide_path[i] = ch == '/' ? '\\' : ch;
    }
    wide_path[path_length] = L'\0';

    EFI_STATUS status = uefi_platform_read_file(wide_path, &data, &size);
    free(wide_path);
    if (EFI_ERROR(status)) {
        return NULL;
    }

    file = malloc(sizeof(*file));
    if (file == NULL) {
        free(data);
        return NULL;
    }
    file->data = data;
    file->size = (size_t)size;
    file->position = 0;
    return (FILE *)file;
}

size_t jvm_file_read(void *buffer, size_t size, size_t count, FILE *stream) {
    UefiMemoryFile *file = (UefiMemoryFile *)stream;
    size_t available;
    size_t bytes_to_read;

    if (buffer == NULL || file == NULL || size == 0 || count == 0 ||
        count > SIZE_MAX / size) {
        return 0;
    }

    available = file->size - file->position;
    bytes_to_read = size * count;
    if (bytes_to_read > available) {
        bytes_to_read = available;
    }
    bytes_to_read -= bytes_to_read % size;
    for (size_t i = 0; i < bytes_to_read; i++) {
        ((unsigned char *)buffer)[i] = file->data[file->position + i];
    }
    file->position += bytes_to_read;
    return bytes_to_read / size;
}

int jvm_file_seek(FILE *stream, long offset, int origin) {
    UefiMemoryFile *file = (UefiMemoryFile *)stream;
    size_t base;
    size_t target;

    if (file == NULL) {
        return -1;
    }
    if (origin == SEEK_SET) {
        base = 0;
    } else if (origin == SEEK_CUR) {
        base = file->position;
    } else if (origin == SEEK_END) {
        base = file->size;
    } else {
        return -1;
    }

    if (offset < 0) {
        size_t distance = (size_t)(-(offset + 1)) + 1;
        if (distance > base) {
            return -1;
        }
        target = base - distance;
    } else {
        if ((size_t)offset > file->size - base) {
            return -1;
        }
        target = base + (size_t)offset;
    }

    file->position = target;
    return 0;
}

long jvm_file_tell(FILE *stream) {
    UefiMemoryFile *file = (UefiMemoryFile *)stream;
    if (file == NULL || file->position > LONG_MAX) {
        return -1;
    }
    return (long)file->position;
}

int jvm_file_eof(FILE *stream) {
    UefiMemoryFile *file = (UefiMemoryFile *)stream;
    return file == NULL || file->position >= file->size;
}

int jvm_file_close(FILE *stream) {
    UefiMemoryFile *file = (UefiMemoryFile *)stream;
    if (file == NULL) {
        return -1;
    }
    free(file->data);
    free(file);
    return 0;
}
