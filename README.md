# RISC-666
A RISC-V user-mode emulator that runs DooM

![DooM Menu 1](https://github.com/lcq2/lcq2.github.io/blob/master/risc_666/risc_666_1.png?raw=true "Shareware screen 1")
![DooM Menu 2](https://github.com/lcq2/lcq2.github.io/blob/master/risc_666/risc_666_2.png?raw=true "Shareware screen 2")
![DooM Menu 3](https://github.com/lcq2/lcq2.github.io/blob/master/risc_666/risc_666_2.png?raw=true "Shareware screen 3")

### RISC-V emulation details
Currently only rv32iam is supported, it's enough for what I need. In the future I will add floating point and compressed instructions to reduce overall code size.

This is a personal toy project, never intented to be a full featured RISC-V emulator, for that I'm working on riscv-emu (which is on hold for now).

### Toolchain details
I'm using this toolchain: https://github.com/riscv/riscv-gnu-toolchain to build target executables. The toolchain is built in newlib mode, without compressed instructions.

Basically this means that printf("%f", ff) will croak, but it's ok for now :D
