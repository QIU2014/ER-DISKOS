#include <efi.h>
#include <efilib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "UEFIJvmPlatform.h"
#include "UEFIPlatform.h"
#include "mini_jvm/mini_jvm/utils/d_type.h"

FILE *stdout = (FILE *)(UINTN)1;
FILE *stderr = (FILE *)(UINTN)2;
static UINT32 random_state = 1;

void *memmove(void *destination, const void *source, size_t count) {
    unsigned char *dst = (unsigned char *)destination;
    const unsigned char *src = (const unsigned char *)source;

    if ((UINTN)dst < (UINTN)src) {
        for (size_t i = 0; i < count; i++) {
            dst[i] = src[i];
        }
    } else if ((UINTN)dst > (UINTN)src) {
        while (count > 0) {
            count--;
            dst[count] = src[count];
        }
    }
    return destination;
}

int strcmp(const char *left, const char *right) {
    while (*left != '\0' && *left == *right) {
        left++;
        right++;
    }
    return (unsigned char)*left - (unsigned char)*right;
}

int vfprintf(FILE *stream, const char *format, va_list args) {
    if (stream != stdout && stream != stderr) {
        return -1;
    }
    return uefi_jvm_vprintf(format, args);
}

int fprintf(FILE *stream, const char *format, ...) {
    va_list args;
    int result;

    va_start(args, format);
    result = vfprintf(stream, format, args);
    va_end(args);
    return result;
}

int printf(const char *format, ...) {
    va_list args;
    int result;

    va_start(args, format);
    result = uefi_jvm_vprintf(format, args);
    va_end(args);
    return result;
}

int rand(void) {
    random_state = random_state * 1103515245U + 12345U;
    return (int)((random_state >> 1) & RAND_MAX);
}

void srand(unsigned int seed) {
    random_state = seed;
}

time_t time(time_t *result) {
    s64 milliseconds = currentTimeMillis();
    if (milliseconds < 0) {
        return (time_t)-1;
    }
    time_t seconds = (time_t)(milliseconds / 1000);
    if (result != NULL) {
        *result = seconds;
    }
    return seconds;
}

void exit(int status) {
    EFI_STATUS exit_status = uefi_platform_exit((INTN)status);
    uefi_jvm_printf("UEFI application exit failed: %llx.\n", (UINT64)exit_status);
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
