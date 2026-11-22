# ER-DISKOS UEFI JVM

The x86-64 UEFI target starts the mini JVM and invokes
`com/eric/Main.main(String[])` from the EFI System Partition. It uses UEFI boot
services, a single execution thread, firmware console I/O, and class files
under `\jvm`. `System.in` reads from the UEFI serial input protocol when
available, otherwise from the firmware keyboard; `System.out` and `System.err`
write to the firmware console. The bundled `Scanner` supports `nextLine()` and
`close()` only. Networking and JDWP are not included; `Thread.start()` and
`Object.wait()` raise `UnsupportedOperationException`. The background GC worker
is disabled; explicit `System.gc()` and shutdown collection remain available.

Build the x86-64 EFI app and a bootable UEFI ISO containing a 64 MiB FAT ESP.
Image creation also requires `mvn`, `javac`, `mkfs.vfat`, `mcopy`, `mmd`,
`mdir`, `truncate`, and `genisoimage` or `mkisofs`. Booting it requires QEMU
and OVMF.

```sh
cmake -S . -B /tmp/er-diskos-uefi-build \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/UEFI-x86_64-toolchain.cmake"
cmake --build /tmp/er-diskos-uefi-build --target uefi_test
```

For an interactive build-and-boot workflow, run:

```sh
scripts/build-and-run-qemu.sh [build-dir]
```

It configures CMake, builds `uefi_iso`, and launches QEMU. The default
build directory is `build-uefi`; set `UEFI_BUILD_DIR` or pass a directory to
change it. Set `UEFI_ISO_IMAGE` to choose another ISO path.

`uefi_test` builds the EFI app and Java application, generates the UEFI ISO,
boots it in QEMU, and checks for the sample greeting. Use `uefi_iso` when you
only want to generate the ISO, then run
`scripts/run-qemu.sh` with that ISO path. When replacing the sample app, set
`UEFI_TEST_EXPECTED_OUTPUT` to text it prints. Remove generated build outputs,
the ISO, and QEMU's saved firmware variables with:

```sh
cmake --build /tmp/er-diskos-uefi-build --target clean
```

The ISO contains an El Torito EFI boot image with `BOOTX64.EFI` at
`EFI/BOOT/BOOTX64.EFI`. The build compiles the mini-JVM runtime sources and
`mini_jvm/javalib/src/sys.properties` into
`mini_jvm/javalib/build/classes`, builds the `javaProject` Maven sub-project,
and copies both class trees into the ISO's `jvm/` directory. Override
`UEFI_JAVA_PROJECT_DIR` to select another Maven application,
`UEFI_JAVA_PROJECT_POM` to select another Maven descriptor,
`UEFI_JAVA_CLASSES_DIR` to select a different compiled mini-JVM class library,
or `UEFI_ISO_IMAGE` to change the ISO location. The application must provide
`com.eric.Main`, and its bytecode must be supported by the mini JVM. The
expected ESP layout is:

```text
EFI/BOOT/BOOTX64.EFI
jvm/sys.properties
jvm/java/...
jvm/com/eric/Main.class
```

The separate `uefi_boot_elf` target produces `UEFISMOKE.EFI` for testing
firmware file access without starting the VM. `com/eric/Main.class` must use
bytecodes supported by the mini JVM.
