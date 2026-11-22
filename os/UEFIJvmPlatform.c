#include "UEFIJvmPlatform.h"

#include <efilib.h>
#include <limits.h>

#include "mini_jvm/mini_jvm/utils/d_type.h"
#include "UEFIPlatform.h"

static s64 days_in_year(UINT16 year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365;
}

static int build_efi_format(const char *format, CHAR8 *efi_format, UINTN capacity) {
    UINTN source = 0;
    UINTN target = 0;

    while (format[source] != '\0') {
        if (target >= capacity - 1) {
            return -1;
        }
        efi_format[target++] = (CHAR8)format[source++];
        if (format[source - 1] != '%') {
            continue;
        }
        if (format[source] == '%') {
            if (target + 1 >= capacity) {
                return -1;
            }
            efi_format[target++] = (CHAR8)format[source++];
            continue;
        }

        int wide_string = 0;
        while (format[source] != '\0') {
            char current = format[source];
            if (target >= capacity - 1) {
                return -1;
            }
            if (current == 'l') {
                wide_string = 1;
                efi_format[target++] = (CHAR8)format[source++];
            } else if (current == 'h' || current == 'j' || current == 'z' ||
                       current == 't' || current == 'L') {
                efi_format[target++] = (CHAR8)format[source++];
            } else if ((current >= '0' && current <= '9') || current == '-' ||
                       current == '+' || current == ' ' || current == '#' ||
                       current == '0' || current == '\'' || current == '.' ||
                       current == '*') {
                efi_format[target++] = (CHAR8)format[source++];
            } else {
                break;
            }
        }

        if (format[source] == '\0') {
            return -1;
        }
        if (target >= capacity - 1) {
            return -1;
        }
        if (format[source] == 's' && !wide_string) {
            efi_format[target++] = 'a';
            source++;
        } else {
            efi_format[target++] = (CHAR8)format[source++];
        }
    }

    efi_format[target] = '\0';
    return 0;
}

static int flush_wide(CHAR16 *buffer, UINTN *length) {
    EFI_STATUS status;
    if (*length == 0) {
        return 0;
    }
    buffer[*length] = L'\0';
    status = uefi_platform_write(buffer);
    if (EFI_ERROR(status)) {
        return -1;
    }
    *length = 0;
    return 0;
}

static int append_codepoint(CHAR16 *buffer, UINTN capacity, UINTN *length,
                            UINT32 codepoint) {
    UINTN units = codepoint <= 0xffff ? 1 : 2;
    if (*length + units >= capacity && flush_wide(buffer, length) != 0) {
        return -1;
    }
    if (codepoint <= 0xffff) {
        buffer[(*length)++] = (CHAR16)codepoint;
    } else {
        codepoint -= 0x10000;
        buffer[(*length)++] = (CHAR16)(0xd800 + (codepoint >> 10));
        buffer[(*length)++] = (CHAR16)(0xdc00 + (codepoint & 0x3ff));
    }
    return 0;
}

static UINT32 decode_utf8(const CHAR8 *text, UINTN remaining, UINTN *consumed) {
    UINT32 codepoint;
    UINTN expected;

    if (text[0] < 0x80) {
        *consumed = 1;
        return text[0];
    }
    if (text[0] >= 0xc2 && text[0] <= 0xdf) {
        codepoint = text[0] & 0x1f;
        expected = 2;
    } else if (text[0] >= 0xe0 && text[0] <= 0xef) {
        codepoint = text[0] & 0x0f;
        expected = 3;
    } else if (text[0] >= 0xf0 && text[0] <= 0xf4) {
        codepoint = text[0] & 0x07;
        expected = 4;
    } else {
        *consumed = 1;
        return 0xfffd;
    }

    if (remaining < expected) {
        *consumed = 1;
        return 0xfffd;
    }
    for (UINTN i = 1; i < expected; i++) {
        if ((text[i] & 0xc0) != 0x80) {
            *consumed = 1;
            return 0xfffd;
        }
        codepoint = (codepoint << 6) | (text[i] & 0x3f);
    }
    if ((expected == 3 && codepoint < 0x800) ||
        (expected == 4 && codepoint < 0x10000) ||
        (codepoint >= 0xd800 && codepoint <= 0xdfff) ||
        codepoint > 0x10ffff) {
        *consumed = 1;
        return 0xfffd;
    }
    *consumed = expected;
    return codepoint;
}

s64 currentTimeMillis(void) {
    static const UINT8 days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    EFI_TIME time;
    EFI_STATUS status;
    s64 days = 0;
    UINTN month;
    UINT16 year;

    if (RT == NULL || RT->GetTime == NULL) {
        return -1;
    }

    status = uefi_call_wrapper(RT->GetTime, 2, &time, NULL);
    if (EFI_ERROR(status) || time.Year < 1970 || time.Month < 1 || time.Month > 12 ||
        time.Day < 1 || time.Hour > 23 || time.Minute > 59 || time.Second > 60 ||
        time.Nanosecond > 999999999) {
        return -1;
    }

    for (year = 1970; year < time.Year; year++) {
        days += days_in_year(year);
    }
    for (month = 1; month < time.Month; month++) {
        days += days_in_month[month - 1];
        if (month == 2 && days_in_year(time.Year) == 366) {
            days++;
        }
    }
    if (time.Day > days_in_month[time.Month - 1] +
                   (time.Month == 2 && days_in_year(time.Year) == 366 ? 1 : 0)) {
        return -1;
    }
    days += time.Day - 1;

    s64 seconds = days * 86400 + time.Hour * 3600 + time.Minute * 60 + time.Second;
    if (time.TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
        seconds -= time.TimeZone * 60;
    }
    return seconds * 1000 + time.Nanosecond / 1000000;
}

s64 nanoTime(void) {
    static s64 last_value = -1;
    s64 milliseconds = currentTimeMillis();
    if (milliseconds < 0 || milliseconds > LLONG_MAX / 1000000) {
        return -1;
    }
    s64 value = milliseconds * 1000000;
    if (value <= last_value) {
        if (last_value == LLONG_MAX) {
            return -1;
        }
        value = last_value + 1;
    }
    last_value = value;
    return value;
}

s64 threadSleep(s64 milliseconds) {
    if (milliseconds < 0 || BS == NULL || BS->Stall == NULL ||
        (UINT64)milliseconds > (UINT64)(((UINTN)-1) / 1000)) {
        return -1;
    }
    EFI_STATUS status = uefi_call_wrapper(BS->Stall, 1, (UINTN)milliseconds * 1000);
    return EFI_ERROR(status) ? -1 : 0;
}

int uefi_jvm_vprintf(const char *format, va_list args) {
    CHAR8 ascii[2048];
    CHAR8 efi_format[2048];
    CHAR16 wide[256];
    const UINTN wide_capacity = sizeof(wide) / sizeof(wide[0]);
    UINTN length;
    UINTN i;
    UINTN written = 0;
    UINT32 previous = 0;

    if (format == NULL) {
        return -1;
    }

    if (build_efi_format(format, efi_format, sizeof(efi_format)) != 0) {
        return -1;
    }
    length = AsciiVSPrint(ascii, sizeof(ascii), efi_format, args);
    if (length >= sizeof(ascii)) {
        return -1;
    }

    for (i = 0; i < length; i++) {
        UINTN consumed;
        UINT32 codepoint = decode_utf8(&ascii[i], length - i, &consumed);
        if (codepoint == '\n' && previous != '\r' &&
            append_codepoint(wide, wide_capacity, &written, '\r') != 0) {
            return -1;
        }
        if (append_codepoint(wide, wide_capacity, &written, codepoint) != 0) {
            return -1;
        }
        previous = codepoint;
        i += consumed - 1;
    }

    if (flush_wide(wide, &written) != 0) {
        return -1;
    }
    return (int)length;
}

int uefi_jvm_printf(const char *format, ...) {
    va_list args;
    int result;

    va_start(args, format);
    result = uefi_jvm_vprintf(format, args);
    va_end(args);
    return result;
}

int uefi_jvm_putchar(UINT16 character) {
    return uefi_jvm_write_utf16(&character, 1);
}

int uefi_jvm_write_utf16(const UINT16 *characters, UINTN count) {
    CHAR16 wide[256];
    const UINTN capacity = sizeof(wide) / sizeof(wide[0]);
    UINTN written = 0;
    UINTN previous = 0;

    if (characters == NULL && count != 0) {
        return -1;
    }

    for (UINTN i = 0; i < count; i++) {
        UINT32 codepoint = characters[i];
        if (codepoint >= 0xd800 && codepoint <= 0xdbff && i + 1 < count &&
            characters[i + 1] >= 0xdc00 && characters[i + 1] <= 0xdfff) {
            codepoint = 0x10000 + ((codepoint - 0xd800) << 10) +
                        (characters[i + 1] - 0xdc00);
            i++;
        }
        if (codepoint == '\n' && previous != '\r' &&
            append_codepoint(wide, capacity, &written, '\r') != 0) {
            return -1;
        }
        if (append_codepoint(wide, capacity, &written, codepoint) != 0) {
            return -1;
        }
        previous = codepoint;
    }

    return flush_wide(wide, &written) == 0 ? (int)count : -1;
}

static int append_unsigned(char *buffer, UINTN capacity, UINTN *length, UINT64 value) {
    char digits[24];
    UINTN digit_count = 0;
    do {
        digits[digit_count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0 && digit_count < sizeof(digits));

    while (digit_count > 0) {
        if (*length + 1 >= capacity) {
            return -1;
        }
        buffer[(*length)++] = digits[--digit_count];
    }
    return 0;
}

int uefi_jvm_format_double(char *buffer, UINTN capacity, double value) {
    UINTN length = 0;
    int negative = value < 0.0;
    double absolute_value = negative ? -value : value;
    UINT64 integer_part;
    UINT64 fractional_part;

    if (buffer == NULL || capacity < 2) {
        return -1;
    }
    if (value != value) {
        if (capacity < 4) {
            return -1;
        }
        buffer[0] = 'N';
        buffer[1] = 'a';
        buffer[2] = 'N';
        buffer[3] = '\0';
        return 3;
    }
    if (absolute_value > 1.7976931348623157e308) {
        const char *infinity = negative ? "-Infinity" : "Infinity";
        UINTN i = 0;
        while (infinity[i] != '\0') {
            if (i + 1 >= capacity) {
                return -1;
            }
            buffer[i] = infinity[i];
            i++;
        }
        buffer[i] = '\0';
        return (int)i;
    }

    if (negative) {
        buffer[length++] = '-';
    }
    if (absolute_value > 18446744073709549568.0) {
        return -1;
    }

    integer_part = (UINT64)absolute_value;
    fractional_part = (UINT64)((absolute_value - (double)integer_part) * 1000000.0 + 0.5);
    if (fractional_part >= 1000000) {
        if (integer_part == (UINT64)-1) {
            return -1;
        }
        integer_part++;
        fractional_part -= 1000000;
    }
    if (append_unsigned(buffer, capacity, &length, integer_part) != 0 ||
        length + 8 >= capacity) {
        return -1;
    }
    buffer[length++] = '.';
    UINT64 divisor = 100000;
    while (divisor != 0) {
        buffer[length++] = (char)('0' + fractional_part / divisor);
        fractional_part %= divisor;
        divisor /= 10;
    }
    buffer[length] = '\0';
    return (int)length;
}
