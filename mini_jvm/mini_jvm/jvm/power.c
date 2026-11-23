#include "power.h"
#include "efibind.h"
#include "efidef.h"
#include "efierr.h"
#include "efilib.h"
#include "efiprot.h"
#include "jvm_util.h"

EFI_STATUS EFIAPI shutdown() {
    if (gRT == NULL || gRT->ResetSystem == NULL) {
        return EFI_NOT_READY;
    }

    uefi_call_wrapper(gRT->ResetSystem, 4, EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    return EFI_DEVICE_ERROR;
}

EFI_STATUS EFIAPI reboot() {
    if (gRT == NULL || gRT->ResetSystem == NULL) {
        return EFI_NOT_READY;
    }

    uefi_call_wrapper(gRT->ResetSystem, 4, EfiResetWarm, EFI_SUCCESS, 0, NULL);
    return EFI_DEVICE_ERROR;
}