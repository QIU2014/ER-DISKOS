#include <stddef.h>
#include <stdint.h>
#define HEAP_ALIGN 16u

typedef struct {
    size_t size;
    size_t pad;
} uefi_alloc_hdr_t;

static size_t align_up_size(size_t val, size_t align);

void *malloc(size_t size);

void free(void *ptr);

void *calloc(size_t nmemb, size_t size);

void *realloc(void *ptr, size_t newSize);