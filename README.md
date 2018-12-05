## RISC-666
A RISC-V user-mode emulator, based on my own riscv-emu, that can run newlib based executables through syscall emulation...more details coming soon :)

### RISC-V emulation details
Currently only rv32iam is supported, it's enough for what I need. In the future I will add floating point and compressed instructions to reduce overall code size.

This is a personal toy project, never intented to be a full featured RISC-V emulator, for that I'm working on riscv-emu (which is on hold for now).

### Toolchain details
I'm using this toolchain: https://github.com/riscv/riscv-gnu-toolchain to build target executables. The toolchain is built in newlib mode, without compressed instructions.

Basically this means that printf("%f", ff) will croak, but it's ok for now :D
