#include "UEFIPlatform.h"

#include <efilib.h>
#include <stdlib.h>

static EFI_FILE_PROTOCOL *volume_root;
static EFI_HANDLE application_image;
static EFI_SERIAL_IO_PROTOCOL *serial_input;
static BOOLEAN skip_line_feed;

EFI_STATUS uefi_platform_init(EFI_HANDLE image_handle) {
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *file_system = NULL;
    EFI_STATUS status;

    if (image_handle == NULL || BS == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    if (volume_root != NULL) {
        return EFI_ALREADY_STARTED;
    }

    status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle,
                               &LoadedImageProtocol, (VOID **)&loaded_image);
    if (EFI_ERROR(status)) {
        return status;
    }
    if (loaded_image == NULL || loaded_image->DeviceHandle == NULL) {
        return EFI_NOT_FOUND;
    }

    status = uefi_call_wrapper(BS->HandleProtocol, 3, loaded_image->DeviceHandle,
                               &FileSystemProtocol, (VOID **)&file_system);
    if (EFI_ERROR(status)) {
        return status;
    }
    if (file_system == NULL || file_system->OpenVolume == NULL) {
        return EFI_UNSUPPORTED;
    }

    status = uefi_call_wrapper(file_system->OpenVolume, 2, file_system, &volume_root);
    if (EFI_ERROR(status) || volume_root == NULL) {
        volume_root = NULL;
        if (!EFI_ERROR(status)) {
            status = EFI_DEVICE_ERROR;
        }
    }
    if (!EFI_ERROR(status)) {
        application_image = image_handle;
        if (BS->LocateProtocol != NULL) {
            EFI_SERIAL_IO_PROTOCOL *protocol = NULL;
            EFI_STATUS serial_status =
                uefi_call_wrapper(BS->LocateProtocol, 3, &SerialIoProtocol,
                                  NULL, (VOID **)&protocol);
            if (!EFI_ERROR(serial_status)) {
                serial_input = protocol;
            }
        }
    }
    return status;
}

EFI_STATUS uefi_platform_write(const CHAR16 *text) {
    if (text == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    if (ST == NULL || ST->ConOut == NULL) {
        return EFI_NOT_READY;
    }
    if (ST->ConOut->OutputString == NULL) {
        return EFI_UNSUPPORTED;
    }
    return uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, (CHAR16 *)text);
}

static EFI_STATUS echo_input_char(CHAR16 character) {
    CHAR16 text[4] = {L'\0', L'\0', L'\0', L'\0'};
    if (character == L'\r' || character == L'\n') {
        text[0] = L'\r';
        text[1] = L'\n';
    } else if (character == L'\b' || character == 0x7f) {
        text[0] = L'\b';
        text[1] = L' ';
        text[2] = L'\b';
    } else if (character >= L' ') {
        text[0] = character;
    } else {
        return EFI_SUCCESS;
    }
    return uefi_platform_write(text);
}

EFI_STATUS uefi_platform_read_char(CHAR16 *character) {
    EFI_SIMPLE_TEXT_IN_PROTOCOL *console_input;
    EFI_INPUT_KEY key;
    EFI_EVENT event;
    UINTN event_index;
    EFI_STATUS status;

    if (character == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    for (;;) {
        if (serial_input != NULL && serial_input->Read != NULL) {
            UINT8 input_byte;
            UINTN input_size = sizeof(input_byte);
            status = uefi_call_wrapper(serial_input->Read, 3, serial_input,
                                       &input_size, &input_byte);
            if (status == EFI_TIMEOUT || status == EFI_NOT_READY) {
                if (BS != NULL && BS->Stall != NULL) {
                    uefi_call_wrapper(BS->Stall, 1, 1000);
                }
                continue;
            }
            if (EFI_ERROR(status)) {
                return status;
            }
            if (input_size == 0) {
                continue;
            }
            key.UnicodeChar = (CHAR16)input_byte;
        } else {
            if (ST == NULL || ST->ConIn == NULL || BS == NULL ||
                BS->WaitForEvent == NULL) {
                return EFI_NOT_READY;
            }
            console_input = ST->ConIn;
            if (console_input->ReadKeyStroke == NULL ||
                console_input->WaitForKey == NULL) {
                return EFI_UNSUPPORTED;
            }

            event = console_input->WaitForKey;
            status = uefi_call_wrapper(BS->WaitForEvent, 3, 1, &event, &event_index);
            if (EFI_ERROR(status)) {
                return status;
            }
            status = uefi_call_wrapper(console_input->ReadKeyStroke, 2,
                                       console_input, &key);
            if (status == EFI_NOT_READY) {
                continue;
            }
            if (EFI_ERROR(status)) {
                return status;
            }
        }

        if (skip_line_feed) {
            skip_line_feed = FALSE;
            if (key.UnicodeChar == L'\n') {
                continue;
            }
        }
        if (key.UnicodeChar == 0) {
            continue;
        }
        if (key.UnicodeChar == L'\r') {
            skip_line_feed = TRUE;
            *character = L'\n';
        } else {
            *character = key.UnicodeChar;
        }
        return echo_input_char(key.UnicodeChar);
    }
}

EFI_STATUS uefi_platform_read_file(const CHAR16 *path, VOID **buffer, UINTN *size) {
    EFI_FILE_PROTOCOL *file = NULL;
    EFI_FILE_INFO *info = NULL;
    EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
    UINTN info_size = 0;
    UINTN file_size;
    UINTN offset = 0;
    VOID *data = NULL;
    EFI_STATUS status;
    EFI_STATUS close_status;

    if (path == NULL || buffer == NULL || size == NULL || volume_root == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    *buffer = NULL;
    *size = 0;

    if (volume_root->Open == NULL) {
        return EFI_UNSUPPORTED;
    }

    status = uefi_call_wrapper(volume_root->Open, 5, volume_root, &file,
                               (CHAR16 *)path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        return status;
    }
    if (file == NULL || file->GetInfo == NULL || file->Read == NULL || file->Close == NULL) {
        if (file != NULL && file->Close != NULL) {
            close_status = uefi_call_wrapper(file->Close, 1, file);
            if (EFI_ERROR(close_status)) {
                Print(L"Warning: unable to close an unsupported EFI file handle.\r\n");
            }
        }
        return EFI_UNSUPPORTED;
    }

    status = uefi_call_wrapper(file->GetInfo, 4, file, &file_info_guid, &info_size, NULL);
    if (status != EFI_BUFFER_TOO_SMALL || info_size < SIZE_OF_EFI_FILE_INFO) {
        if (!EFI_ERROR(status)) {
            status = EFI_COMPROMISED_DATA;
        }
        goto cleanup;
    }

    info = malloc(info_size);
    if (info == NULL) {
        status = EFI_OUT_OF_RESOURCES;
        goto cleanup;
    }

    status = uefi_call_wrapper(file->GetInfo, 4, file, &file_info_guid, &info_size, info);
    if (EFI_ERROR(status)) {
        goto cleanup;
    }
    if (info->FileSize > (UINT64)(UINTN)-1) {
        status = EFI_BAD_BUFFER_SIZE;
        goto cleanup;
    }

    file_size = (UINTN)info->FileSize;
    data = malloc(file_size == 0 ? 1 : file_size);
    if (data == NULL) {
        status = EFI_OUT_OF_RESOURCES;
        goto cleanup;
    }

    while (offset < file_size) {
        UINTN bytes_read = file_size - offset;
        status = uefi_call_wrapper(file->Read, 3, file, &bytes_read,
                                   (UINT8 *)data + offset);
        if (EFI_ERROR(status)) {
            goto cleanup;
        }
        if (bytes_read == 0) {
            status = EFI_END_OF_FILE;
            goto cleanup;
        }
        if (bytes_read > file_size - offset) {
            status = EFI_DEVICE_ERROR;
            goto cleanup;
        }
        offset += bytes_read;
    }

    status = EFI_SUCCESS;

cleanup:
    free(info);
    close_status = uefi_call_wrapper(file->Close, 1, file);
    if (EFI_ERROR(close_status)) {
        if (EFI_ERROR(status)) {
            Print(L"Warning: file close failed while handling another file error.\r\n");
        } else {
            status = close_status;
        }
    }
    if (EFI_ERROR(status)) {
        free(data);
        return status;
    }

    *buffer = data;
    *size = file_size;
    return EFI_SUCCESS;
}

EFI_STATUS uefi_platform_shutdown(void) {
    EFI_FILE_PROTOCOL *root = volume_root;

    if (root == NULL) {
        return EFI_SUCCESS;
    }
    if (root->Close == NULL) {
        return EFI_UNSUPPORTED;
    }

    volume_root = NULL;
    return uefi_call_wrapper(root->Close, 1, root);
}

EFI_STATUS uefi_platform_exit(INTN status) {
    if (application_image == NULL || BS == NULL || BS->Exit == NULL) {
        return EFI_NOT_READY;
    }
    EFI_STATUS close_status = uefi_platform_shutdown();
    if (EFI_ERROR(close_status)) {
        return close_status;
    }
    return uefi_call_wrapper(BS->Exit, 4, application_image, (EFI_STATUS)status, 0, NULL);
}
