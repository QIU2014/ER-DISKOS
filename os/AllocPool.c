#include "AllocPool.h"
#include <stddef.h>
#include <stdint.h>
#include <efi.h>
#include <efilib.h>

static size_t align_up_size(size_t val, size_t align) {
    if (align == 0) {
        return val;
    }

    size_t rem = val % align;
    if (rem == 0) {
        return val;
    }
    size_t padding = align - rem;
    if (val > SIZE_MAX - padding) {
        return 0;
    }
    return val + padding;
}

void *malloc(size_t size) {
    if (size == 0) {
        size = 1;
    }

    if (!BS || !BS->AllocatePool) {
        return NULL;
    }

    if (size > SIZE_MAX - sizeof(uefi_alloc_hdr_t) - HEAP_ALIGN) {
        return NULL;
    }

    size_t total = size + sizeof(uefi_alloc_hdr_t) + HEAP_ALIGN;
    VOID *raw = NULL;
    EFI_STATUS Status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, total, &raw);
    if (EFI_ERROR(Status) || raw == NULL) {
        return NULL;
    }

    uintptr_t raw_addr = (uintptr_t) raw;
    if (raw_addr > UINTPTR_MAX - sizeof(uefi_alloc_hdr_t)) {
        uefi_call_wrapper(BS->FreePool, 1, raw);
        return NULL;
    }

    size_t aligned = align_up_size((size_t)(raw_addr + sizeof(uefi_alloc_hdr_t)), HEAP_ALIGN);
    if (aligned == 0) {
        uefi_call_wrapper(BS->FreePool, 1, raw);
        return NULL;
    }
    uintptr_t payload_addr = (uintptr_t) aligned;

    uefi_alloc_hdr_t *hdr = (uefi_alloc_hdr_t *)(payload_addr - sizeof(uefi_alloc_hdr_t));
    hdr->size = size;
    hdr->pad = (size_t)(payload_addr - raw_addr);

    return (void *)payload_addr;
}

void free(void *ptr) {
    if (!ptr) return;
    if (!BS || !BS->FreePool) return;

    uintptr_t payload_addr = (uintptr_t)ptr;
    uefi_alloc_hdr_t *hdr = (uefi_alloc_hdr_t *)(payload_addr - sizeof(uefi_alloc_hdr_t));
    uintptr_t raw_addr = payload_addr - hdr->pad;

    uefi_call_wrapper(BS->FreePool, 1, (VOID *)raw_addr);
}

void *calloc(size_t nmemb, size_t size) {
    if (nmemb != 0 && size > SIZE_MAX / nmemb) {
        return NULL;
    }
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (!ptr) {
        return NULL;
    }

    uint8_t *dst = (uint8_t *)ptr;
    for (size_t i = 0; i < total; i++) {
        dst[i] = 0;
    }
    return ptr;
}

void *realloc(void *ptr, size_t new_size) {
    if (!ptr) {
        return malloc(new_size);
    }
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    uintptr_t payload_addr = (uintptr_t)ptr;
    uefi_alloc_hdr_t *hdr = (uefi_alloc_hdr_t *)(payload_addr - sizeof(uefi_alloc_hdr_t));
    size_t old_size = hdr->size;

    void *next = malloc(new_size);
    if (!next) {
        return NULL;
    }

    size_t copy_len = old_size < new_size ? old_size : new_size;
    const uint8_t *src = (const uint8_t *)ptr;
    uint8_t *dst = (uint8_t *)next;
    for (size_t i = 0; i < copy_len; i++) {
        dst[i] = src[i];
    }

    free(ptr);
    return next;
}