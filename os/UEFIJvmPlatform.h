#ifndef ER_DISKOS_UEFI_JVM_PLATFORM_H
#define ER_DISKOS_UEFI_JVM_PLATFORM_H

#include <efi.h>
#include <stdarg.h>

#include "mini_jvm/mini_jvm/utils/d_type.h"

s64 currentTimeMillis(void);
s64 nanoTime(void);
s64 threadSleep(s64 milliseconds);
int uefi_jvm_printf(const char *format, ...);
int uefi_jvm_vprintf(const char *format, va_list args);
int uefi_jvm_putchar(UINT16 character);
int uefi_jvm_write_utf16(const UINT16 *characters, UINTN count);
int uefi_jvm_format_double(char *buffer, UINTN capacity, double value);

#endif
