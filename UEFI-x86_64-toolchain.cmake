set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(GNU_EFI_INC /usr/include/efi)
set(GNU_EFI_LIB /usr/lib)

add_compile_options(
    -ffreestanding
    -fno-stack-protector
    -fpic
    -fshort-wchar
    -mno-red-zone
    -Wall
    -Wextra
)

include_directories(
    ${GNU_EFI_INC}
    ${GNU_EFI_INC}/x86_64
    ${GNU_EFI_INC}/protocol
)