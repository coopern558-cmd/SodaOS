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
```

## Screenshots

Login Screen

<img width="1278" height="797" alt="image" src="https://github.com/user-attachments/assets/00e0f0c3-f9f4-4e63-9dea-318ec9b6f0e8" />

Home Page

<img width="1280" height="801" alt="image" src="https://github.com/user-attachments/assets/70160bbb-07f7-4bc9-8660-b1cdcecdb1af" />

Terminal

<img width="1278" height="797" alt="image" src="https://github.com/user-attachments/assets/67a38b32-146f-4ba8-aa8d-3c66424f66fd" />

Files

<img width="1278" height="797" alt="image" src="https://github.com/user-attachments/assets/4afa9320-144f-4a53-9a8e-f009b6482589" />

Notes

<img width="1280" height="802" alt="image" src="https://github.com/user-attachments/assets/9f064b05-3229-422f-9b81-1ac5fffc582c" />

Settings

<img width="1275" height="795" alt="image" src="https://github.com/user-attachments/assets/6819bd91-c38b-42c9-a595-1e0e9badc2b7" />
