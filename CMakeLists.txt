cmake_minimum_required(VERSION 3.16)
project(ER_DISKOS_UEFI C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(GNU_EFI_INCLUDE_DIR "/usr/include/efi" CACHE PATH "GNU-EFI headers")
set(GNU_EFI_LIBRARY_DIR "/usr/lib" CACHE PATH "GNU-EFI libraries")
set(GNU_EFI_CRT0 "${GNU_EFI_LIBRARY_DIR}/crt0-efi-x86_64.o" CACHE FILEPATH "GNU-EFI CRT object")
set(GNU_EFI_LINKER_SCRIPT "${GNU_EFI_LIBRARY_DIR}/elf_x86_64_efi.lds" CACHE FILEPATH "GNU-EFI linker script")

find_program(UEFI_OBJCOPY NAMES objcopy)
if(NOT UEFI_OBJCOPY)
    message(FATAL_ERROR "objcopy is required to produce the EFI PE image")
endif()

function(configure_uefi_image target output_name)
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/os/uefi_include"
        "${GNU_EFI_INCLUDE_DIR}"
        "${GNU_EFI_INCLUDE_DIR}/x86_64"
        "${GNU_EFI_INCLUDE_DIR}/protocol"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/mini_jvm/mini_jvm"
    )

    target_compile_options(${target} PRIVATE
        -D_FORTIFY_SOURCE=0
        -ffreestanding
        -fno-stack-protector
        -fpic
        -fshort-wchar
        -mno-red-zone
        -Wall
        -Wextra
    )

    target_link_options(${target} PRIVATE
        -nostdlib
        -shared
        -Wl,-Bsymbolic
        -Wl,--no-undefined
        -Wl,-znocombreloc
        "-Wl,-T,${GNU_EFI_LINKER_SCRIPT}"
    )

    target_link_libraries(${target} PRIVATE
        "${GNU_EFI_CRT0}"
        "${GNU_EFI_LIBRARY_DIR}/libgnuefi.a"
        "${GNU_EFI_LIBRARY_DIR}/libefi.a"
    )

    set_target_properties(${target} PROPERTIES SUFFIX ".so")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${UEFI_OBJCOPY}"
            --input-target=elf64-x86-64
            -j .text -j .sdata -j .data -j .dynamic -j .dynsym
            -j .rel -j .rela -j .reloc
            --output-target=pei-x86-64
            --subsystem=10
            "$<TARGET_FILE:${target}>"
            "${CMAKE_CURRENT_BINARY_DIR}/${output_name}"
        BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${output_name}"
        VERBATIM
    )
endfunction()

add_executable(uefi_boot_elf
    main.c
    os/AllocPool.c
    os/UEFIPlatform.c
    os/UEFIJvmPlatform.c
    mini_jvm/mini_jvm/jvm/jvm_file_uefi.c
)
configure_uefi_image(uefi_boot_elf UEFISMOKE.EFI)

set(UEFI_JVM_SOURCES
    mini_jvm/mini_jvm/utils/d_utils.c
    mini_jvm/mini_jvm/utils/arraylist.c
    mini_jvm/mini_jvm/utils/hashtable.c
    mini_jvm/mini_jvm/utils/utf8_string.c
    mini_jvm/mini_jvm/utils/pairlist.c
    mini_jvm/mini_jvm/utils/hashset.c
    mini_jvm/mini_jvm/utils/linkedlist.c
    mini_jvm/mini_jvm/utils/spinlock.c
    mini_jvm/mini_jvm/jvm/interpreter.c
    mini_jvm/mini_jvm/jvm/jvm.c
    mini_jvm/mini_jvm/jvm/class.c
    mini_jvm/mini_jvm/jvm/runtime.c
    mini_jvm/mini_jvm/jvm/garbage.c
    mini_jvm/mini_jvm/jvm/jvm_util.c
    mini_jvm/mini_jvm/jvm/jvm_file_uefi.c
    mini_jvm/mini_jvm/jvm/global.c
    mini_jvm/mini_jvm/jvm/resolution.c
    mini_jvm/mini_jvm/jvm/java_native_std.c
    mini_jvm/mini_jvm/jvm/java_native_reflect.c
    mini_jvm/mini_jvm/jvm/class_load.c
)

add_executable(uefi_jvm_elf
    main.c
    os/AllocPool.c
    os/UEFIPlatform.c
    os/UEFIJvmPlatform.c
    os/UEFILibc.c
    os/UEFIMath.c
    ${UEFI_JVM_SOURCES}
)
target_compile_definitions(uefi_jvm_elf PRIVATE JVM_UEFI=1 JVM_UEFI_RUN_VM=1)
configure_uefi_image(uefi_jvm_elf BOOTX64.EFI)

set(UEFI_JAVA_CLASSES_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/mini_jvm/javalib/build/classes"
    CACHE PATH "Compiled mini-JVM class library to place on the ESP")
set(UEFI_JAVA_PROJECT_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/javaProject"
    CACHE PATH "Maven sub-project containing the Java application")
set(UEFI_JAVA_PROJECT_POM
    "${UEFI_JAVA_PROJECT_DIR}/pom.xml"
    CACHE FILEPATH "Maven project descriptor for the Java application")
set(UEFI_ISO_IMAGE
    "${CMAKE_CURRENT_BINARY_DIR}/ER-DISKOS.iso"
    CACHE FILEPATH "Generated bootable UEFI ISO")
set(UEFI_ESP_SIZE_MIB 64 CACHE STRING "UEFI ESP image size in MiB")
set(UEFI_TEST_EXPECTED_OUTPUT "Welcome to ER-DISKOS"
    CACHE STRING "Text the QEMU boot test expects from com/eric/Main")
set(UEFI_QEMU_TEST_TIMEOUT 15 CACHE STRING "QEMU boot test timeout in seconds")

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
    "${UEFI_ISO_IMAGE}"
    "${CMAKE_CURRENT_BINARY_DIR}/OVMF_VARS_ISO.fd"
    "${CMAKE_CURRENT_BINARY_DIR}/uefi-app-classes"
    "${CMAKE_CURRENT_BINARY_DIR}/uefi-app-sources.txt"
)

find_program(UEFI_MAVEN NAMES mvn)
find_program(UEFI_JAVAC NAMES javac)
find_program(UEFI_MKFS_VFAT NAMES mkfs.vfat)
find_program(UEFI_MTOOLS_MCOPY NAMES mcopy)
find_program(UEFI_MTOOLS_MMD NAMES mmd)
find_program(UEFI_MTOOLS_MDIR NAMES mdir)
find_program(UEFI_TRUNCATE NAMES truncate)
find_program(UEFI_MKISOFS NAMES genisoimage mkisofs)

if(UEFI_MAVEN AND UEFI_JAVAC AND UEFI_MKFS_VFAT AND UEFI_MTOOLS_MCOPY AND
    UEFI_MTOOLS_MMD AND UEFI_MTOOLS_MDIR AND UEFI_TRUNCATE AND UEFI_MKISOFS)
    file(GLOB_RECURSE UEFI_JAVA_PROJECT_SOURCES CONFIGURE_DEPENDS
        "${UEFI_JAVA_PROJECT_DIR}/src/*.java")
    file(GLOB_RECURSE UEFI_RUNTIME_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/mini_jvm/javalib/src/*.java")

    set(UEFI_APP_CLASSES_DIR "${CMAKE_CURRENT_BINARY_DIR}/uefi-app-classes")
    set(UEFI_MAVEN_CLASSES_DIR "${UEFI_JAVA_PROJECT_DIR}/target/classes")
    set(UEFI_MAIN_CLASS "${UEFI_APP_CLASSES_DIR}/com/eric/Main.class")
    set(UEFI_RUNTIME_PROPERTIES "${UEFI_JAVA_CLASSES_DIR}/sys.properties")

    add_custom_command(
        OUTPUT "${UEFI_RUNTIME_PROPERTIES}"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${UEFI_JAVA_CLASSES_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${UEFI_JAVA_CLASSES_DIR}"
        COMMAND "${UEFI_JAVAC}" --release 8 -d "${UEFI_JAVA_CLASSES_DIR}"
            ${UEFI_RUNTIME_SOURCES}
        COMMAND "${CMAKE_COMMAND}" -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/mini_jvm/javalib/src/sys.properties"
            "${UEFI_RUNTIME_PROPERTIES}"
        DEPENDS ${UEFI_RUNTIME_SOURCES}
            "${CMAKE_CURRENT_SOURCE_DIR}/mini_jvm/javalib/src/sys.properties"
        VERBATIM
    )
    add_custom_target(uefi_runtime_classes DEPENDS "${UEFI_RUNTIME_PROPERTIES}")

    add_custom_command(
        OUTPUT "${UEFI_MAIN_CLASS}"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${UEFI_APP_CLASSES_DIR}"
        COMMAND "${UEFI_MAVEN}" -q -f "${UEFI_JAVA_PROJECT_POM}" package
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${UEFI_MAVEN_CLASSES_DIR}" "${UEFI_APP_CLASSES_DIR}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS "${UEFI_JAVA_PROJECT_POM}" ${UEFI_JAVA_PROJECT_SOURCES}
        VERBATIM
    )
    add_custom_target(uefi_main_class DEPENDS "${UEFI_MAIN_CLASS}")

    add_custom_target(uefi_iso
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/create-uefi-iso.sh"
            "${UEFI_ISO_IMAGE}"
            "${UEFI_ESP_SIZE_MIB}"
            "${CMAKE_CURRENT_BINARY_DIR}/BOOTX64.EFI"
            "${UEFI_JAVA_CLASSES_DIR}"
            "${UEFI_APP_CLASSES_DIR}"
            "${UEFI_MKISOFS}"
        DEPENDS uefi_jvm_elf uefi_main_class uefi_runtime_classes
        BYPRODUCTS "${UEFI_ISO_IMAGE}"
        COMMENT "Creating bootable UEFI ISO ${UEFI_ISO_IMAGE}"
        VERBATIM
    )

    add_custom_target(uefi_test
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-qemu.sh"
            "${UEFI_ISO_IMAGE}"
            "${UEFI_TEST_EXPECTED_OUTPUT}"
            "${UEFI_QEMU_TEST_TIMEOUT}"
        DEPENDS uefi_iso
        COMMENT "Booting the generated UEFI ISO in QEMU and checking JVM output"
        VERBATIM
    )
else()
    message(STATUS
        "uefi_iso requires mvn, javac, mkfs.vfat, mcopy, mmd, mdir, truncate, and genisoimage or mkisofs")
endif()
