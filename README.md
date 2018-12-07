# ⛧RISC-666⛧
A RISC-V user-mode emulator that runs DooM

![DooM Menu 1](https://github.com/lcq2/lcq2.github.io/blob/master/risc_666/risc_666_1.png?raw=true "Shareware screen 1")
![DooM Menu 2](https://github.com/lcq2/lcq2.github.io/blob/master/risc_666/risc_666_2.png?raw=true "Shareware screen 2")
![DooM Menu 3](https://github.com/lcq2/lcq2.github.io/blob/master/risc_666/risc_666_3.png?raw=true "Shareware screen 3")
![DooM video](https://github.com/lcq2/lcq2.github.io/blob/master/risc_666/risc_666.gif?raw=true "Video")

(the video is slow because of gif conversion)
## ⛧Introduction⛧
Some months ago I came across the "new" RISC-V architecture and I found it very interesting. So I started to play around with it a bit and I discovered that I really liked it, mostly for its simplicity. So the first idea that came into my mind was of course to port DooM to it. But I needed a system where to run code, of course buying a RISC-V dev board or using qemu is too easy and trivial, so I choose to write my own RISC-V emulator and be sure that it could at least run DooM, as a general benchmark.

First I wrote a general-purpose emulator, that you can find in my "trashcan" repo, to avoid being distracted by things not related to CPU emulation. Then I took out the CPU emulator from it and adapted it to run a generic ELF binary.

The emulation model I choose is similar to "qemu user emulation", basically I'm only emulating the CPU itself, everything else is execute on the host operating system (i.e. I emulate all the syscalls).

## ⛧DooM version used⛧
This port of DooM is based on https://github.com/makava/sdldoom-1.10-mod, ported to SDL2. It's a very old port of DooM to SDL1.2, but I needed some reference implementation that was "legacy" enough to be still based around direct framebuffer access, instead of modern OpenGL ports.

All graphics-related code runs in the CPU emulator of course, but SDL initialization and frame update happen on the host.
Basically this means that from the point of view of DooM running in my emulator, the framebuffer is just a malloc'ed buffer, that gets pushed to the host through a syscall.

See rv_av_api.h and rv_av_api.c for more details.

## ⛧Missing⛧
- ~~Input handling~~
- Audio
- Network
- Support for custom WADs and DooM mods in general
- Support for Hexen, Heretic
- A lot...

hey this is just an experiment :D

## ⛧How to use it⛧
You need a RISC-V toolchain based off newlib. Ideally the one you can find here: https://github.com/riscv/riscv-gnu-toolchain.
To build it:
```console
[user@desktop ~]$ ./configure --prefix=/opt/toolchains/riscv32 --with-arch=rv32g --with-abi=ilp32d
[user@desktop ~]$ make
```
*BE SURE TO NOT USE COMPRESSED INSTRUCTION (YOU MUST USE RV32G!!!) !!!*

When the toolchain is ready, be sure to have it in your path, then inside sdldoom directory just do "make". This will create a "doom" executable. Copy "doom" executable and "doom1.wad" in the same folder where "risc_666" executable is located.
Then just do:
```console
[user@desktop ~]$ ./risc_666 doom
```
and DooM should start. Be sure to have SDL2 before building risc_666.
To exit the emulation...send a SIGKILL to the process :D

## ⛧Coming soon⛧
- ~~Input handling~~
- Audio
- Scaled "hires" mode

## ⛧About doom1.wad⛧
This is the shareware demo of course. It's the only thing I have right now to test it.

## ⛧RISC-V emulation details⛧
Currently only rv32iam is supported, it's enough for what I need. In the future I will add floating point and compressed instructions to reduce overall code size.

This is a personal toy project, never intented to be a full featured RISC-V emulator, for that I'm working on riscv-emu (which is on hold for now).

### ⛧Toolchain details⛧
I'm using this toolchain: https://github.com/riscv/riscv-gnu-toolchain to build target executables. The toolchain is built in newlib mode, without compressed instructions.

Basically this means that printf("%f", ff) will croak, but it's ok for now :D

Using newlib means that basically network support, and other stuff, will never be there. This means that in the near future I will update to "musl", however that's not a priority right now. But updating to "musl" will make syscall emulation much, much easier.
