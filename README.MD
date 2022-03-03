# Setup
See :  https://dev.to/younup/cmake-on-stm32-the-beginning-3766
- Download toolchain https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
- Unzip to $HOME/arm


# Platformio hacking

See library.json in /lib

Only one source dir

```
mkdir src
cd src
ln -s ../Core Core 
ln -s ../Middlewares/ Middlewares
ln -s ../Drivers/ Drivers
cd Core/Startup/
mv startup_stm32f103c8tx.s startup_stm32f103c8tx.S
```
