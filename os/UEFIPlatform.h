#ifndef ER_DISKOS_UEFI_PLATFORM_H
#define ER_DISKOS_UEFI_PLATFORM_H

#include <efi.h>

EFI_STATUS uefi_platform_init(EFI_HANDLE image_handle);
EFI_STATUS uefi_platform_write(const CHAR16 *text);
EFI_STATUS uefi_platform_read_char(CHAR16 *character);
EFI_STATUS uefi_platform_read_file(const CHAR16 *path, VOID **buffer, UINTN *size);
EFI_STATUS uefi_platform_shutdown(void);
EFI_STATUS uefi_platform_exit(INTN status);

#endif
