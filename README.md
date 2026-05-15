# SodaOS

SodaOS is a beginner-friendly custom operating system project in early development.

It is a tiny hobby OS that boots in QEMU using GRUB + Multiboot and runs a simple desktop-style shell with a terminal, notes, files view, and settings.

## Current Status

SodaOS is in **early prototype** stage.

### What currently works

- Bootable ISO build with GRUB + Multiboot
- Custom kernel entry and freestanding build pipeline
- Desktop-style UI shell
- Keyboard-based navigation
- Terminal app with basic commands
- RAM-based file system behavior
- Notes editing in-session
- RTC clock display in UI top bar
- Runs in QEMU from WSL

### What is not implemented yet

- Real persistent disk filesystem
- Login/accounts/permissions
- Networking (Wi-Fi/Ethernet/Bluetooth)
- Audio output
- Process scheduler / multitasking
- Hardware drivers beyond current basic input path

## Requirements

Recommended environment:

- Windows + WSL2 (Ubuntu)
- QEMU
- GRUB tools
- GCC/binutils for 32-bit freestanding build

Install dependencies in Ubuntu/WSL:

```bash
sudo apt update
sudo apt install -y build-essential gcc-multilib binutils nasm grub-pc-bin xorriso qemu-system-x86
