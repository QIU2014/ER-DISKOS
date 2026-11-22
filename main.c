#include <efi.h>
#include <efiapi.h>
#include <efierr.h>
#include <efilib.h>
#include <stdarg.h>

#include "jvm/jvm_file.h"
#ifdef JVM_UEFI_RUN_VM
#include "jvm/jvm.h"
#endif
#include "os/UEFIJvmPlatform.h"
#include "os/UEFIPlatform.h"

static EFI_STATUS PrintWithAttribute(UINTN attribute, CONST CHAR16 *format, ...) {
    if (format == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    if (ST == NULL || ST->ConOut == NULL || ST->ConOut->Mode == NULL) {
        return EFI_NOT_READY;
    }
    if (ST->ConOut->SetAttribute == NULL) {
        return EFI_UNSUPPORTED;
    }

    SIMPLE_TEXT_OUTPUT_INTERFACE *console = ST->ConOut;
    EFI_STATUS status =
        uefi_call_wrapper(console->SetAttribute, 2, console, attribute);
    if (EFI_ERROR(status)) {
        return status;
    }

    va_list args;
    va_start(args, format);
    VPrint(format, args);
    va_end(args);

    return uefi_call_wrapper(
        console->SetAttribute, 2, console,
        EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BACKGROUND_BLACK));
}

static EFI_STATUS shutdown_with_status(EFI_STATUS status) {
    EFI_STATUS shutdown_status = uefi_platform_shutdown();
    if (EFI_ERROR(shutdown_status)) {
        Print(L"Unable to close the EFI System Partition.\r\n");
    }
    return EFI_ERROR(status) ? status : shutdown_status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    if (SystemTable == NULL || SystemTable->ConOut == NULL ||
        SystemTable->ConOut->ClearScreen == NULL) {
        return EFI_NOT_READY;
    }
    EFI_STATUS status = uefi_call_wrapper(
        SystemTable->ConOut->ClearScreen, 1, SystemTable->ConOut);
    if (EFI_ERROR(status)) {
        return status;
    }

    status = PrintWithAttribute(
        EFI_TEXT_ATTR(EFI_LIGHTGREEN, EFI_BACKGROUND_BLACK), L"\r\nER-DISKOS UEFI Bootloader\r\n");
    if (EFI_ERROR(status)) {
        return status;
    }

    Print(L"Copyright (C) 2026 Eric Qiu.\r\n\r\nThis program is free software: you can redistribute it and/or modify \r\nit under the terms of the GNU General Public License as published by \r\nthe Free Software Foundation, either version 3 of the License, or \r\n(at your option) any later version.\r\n\r\nThis program is distributed in the hope that it will be useful, \r\nbut WITHOUT ANY WARRANTY; without even the implied warranty of \r\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the \r\nGNU General Public License for more details.\r\n\r\nYou should have received a copy of the GNU General Public License \r\nalong with this program. If not, see <https://www.gnu.org/licenses/>.\r\n\r\n\r\n");

    Print(L"Booting JVM!\r\n");

    status = uefi_platform_init(ImageHandle);
    if (EFI_ERROR(status)) {
        Print(L"Unable to access the EFI System Partition.\r\n");
        return status;
    }

    status = uefi_platform_write(L"UEFI filesystem and console initialized.\r\n");
    if (EFI_ERROR(status)) {
        return shutdown_with_status(status);
    }
    if (uefi_jvm_printf("JVM console formatter ready.\n") < 0) {
        return shutdown_with_status(EFI_DEVICE_ERROR);
    }

#ifdef JVM_UEFI_RUN_VM
    status = uefi_platform_write(L"Starting com/eric/Main from the ESP.\r\n");
    if (EFI_ERROR(status)) {
        return shutdown_with_status(status);
    }
    s32 vm_status = execute("\\jvm\\", "com/eric/Main", 0, NULL);
    if (vm_status != 0) {
        uefi_jvm_printf("JVM execution failed with status %d.\n", vm_status);
    }
    
    gRT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    
    return shutdown_with_status(vm_status == 0 ? EFI_SUCCESS : EFI_ABORTED);
#else
    FILE *class_file = jvm_file_open("\\jvm\\com\\eric\\Main.class", "rb");
    if (class_file == NULL) {
        EFI_STATUS output_status =
            uefi_platform_write(L"Unable to load \\jvm\\com\\eric\\Main.class.\r\n");
        return shutdown_with_status(EFI_ERROR(output_status) ? output_status : EFI_NOT_FOUND);
    }

    UINT8 class_magic[4];
    size_t magic_bytes_read = jvm_file_read(class_magic, 1, sizeof(class_magic), class_file);
    int close_status = jvm_file_close(class_file);
    if (magic_bytes_read != sizeof(class_magic) ||
        class_magic[0] != 0xCA ||
        class_magic[1] != 0xFE ||
        class_magic[2] != 0xBA ||
        class_magic[3] != 0xBE) {
        EFI_STATUS output_status =
            uefi_platform_write(L"Main.class has an invalid class-file magic value.\r\n");
        EFI_STATUS error_status = EFI_ERROR(output_status)
                                  ? output_status
                                  : EFI_COMPROMISED_DATA;
        if (close_status != 0) {
            EFI_STATUS close_output_status =
                uefi_platform_write(L"Unable to close Main.class.\r\n");
            error_status = EFI_ERROR(close_output_status)
                           ? close_output_status
                           : EFI_DEVICE_ERROR;
        }
        return shutdown_with_status(error_status);
    }
    if (close_status != 0) {
        status = uefi_platform_write(L"Unable to close Main.class.\r\n");
        return shutdown_with_status(EFI_ERROR(status) ? status : EFI_DEVICE_ERROR);
    }

    status = uefi_platform_write(L"Main.class magic verified; VM execution is the next step.\r\n");

    gRT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

    return shutdown_with_status(status);
#endif
}