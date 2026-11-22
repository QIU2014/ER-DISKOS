#include "resolution.h"
#include "efibind.h"
#include "efidef.h"
#include "efierr.h"
#include "efilib.h"
#include "efiprot.h"
#include "jvm_util.h"

static EFI_STATUS reset_text_console(void) {
    EFI_SIMPLE_TEXT_OUT_PROTOCOL *console;
    UINTN selected_mode = (UINTN)-1;
    UINTN columns;
    UINTN rows;
    EFI_STATUS status;

    if (ST == NULL || ST->ConOut == NULL || ST->ConOut->Mode == NULL ||
        ST->ConOut->QueryMode == NULL || ST->ConOut->SetMode == NULL ||
        ST->ConOut->ClearScreen == NULL) {
        return EFI_UNSUPPORTED;
    }

    console = ST->ConOut;
    if (console->Mode->Mode >= 0 && console->Mode->Mode < console->Mode->MaxMode &&
        !EFI_ERROR(uefi_call_wrapper(console->QueryMode, 4, console,
                                     (UINTN)console->Mode->Mode, &columns, &rows))) {
        selected_mode = (UINTN)console->Mode->Mode;
    } else {
        for (UINTN mode = 0; mode < (UINTN)console->Mode->MaxMode; mode++) {
            status = uefi_call_wrapper(console->QueryMode, 4, console, mode,
                                       &columns, &rows);
            if (!EFI_ERROR(status)) {
                selected_mode = mode;
                break;
            }
        }
    }
    if (selected_mode == (UINTN)-1) {
        return EFI_UNSUPPORTED;
    }

    status = uefi_call_wrapper(console->SetMode, 2, console, selected_mode);
    if (EFI_ERROR(status)) {
        return status;
    }
    status = uefi_call_wrapper(console->ClearScreen, 1, console);
    if (EFI_ERROR(status)) {
        return status;
    }
    if (console->SetCursorPosition != NULL) {
        status = uefi_call_wrapper(console->SetCursorPosition, 3, console,
                                   (UINTN)0, (UINTN)0);
        if (EFI_ERROR(status)) {
            return status;
        }
    }
    if (console->EnableCursor != NULL) {
        status = uefi_call_wrapper(console->EnableCursor, 2, console, TRUE);
    }
    return status;
}

EFI_STATUS EFIAPI setResolution(s32 width, s32 height) {
    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;

    Status = uefi_call_wrapper(BS->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&gop);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate GOP: %r\n", Status);
        return Status;
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    UINT32 CurrentMode = (gop->Mode == NULL) ? 0 : gop->Mode->Mode;

    Status = uefi_call_wrapper(gop->QueryMode, 4, gop, CurrentMode, &SizeOfInfo, &Info);
    if (Status == EFI_NOT_STARTED) {
        Status = uefi_call_wrapper(gop->SetMode, 2, gop, 0);
    }

    if (EFI_ERROR(Status)) {
        Print(L"Failed to initialize GOP mode: %r\n", Status);
        return Status;
    }
    FreePool(Info);

    for (UINT32 Mode = 0; Mode < gop->Mode->MaxMode; Mode++) {
        Status = uefi_call_wrapper(gop->QueryMode, 4, gop, Mode, &SizeOfInfo, &Info);
        if (EFI_ERROR(Status)) {
            continue;
        }

        if (Info->HorizontalResolution == width && Info->VerticalResolution == height) {
            FreePool(Info);
            Status = uefi_call_wrapper(gop->SetMode, 2, gop, Mode);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to set mode %u: %r\n", Mode, Status);
                return Status;
            }

            EFI_STATUS console_status = reset_text_console();
            if (EFI_ERROR(console_status) && ST != NULL && ST->ConOut != NULL &&
                ST->ConOut->OutputString != NULL) {
                CHAR16 refresh[] = L"\r\n";
                uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, refresh);
            }

            return EFI_SUCCESS;
        }

        FreePool(Info);
    }

    Print(L"Resolution %ux%u not supported.\n", width, height);
    return EFI_UNSUPPORTED;
}