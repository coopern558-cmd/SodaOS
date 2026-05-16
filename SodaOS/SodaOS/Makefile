CC      := gcc
AS      := as
LD      := ld
GRUBMK  := grub-mkrescue
GRUBFILE := grub-file
QEMU    := qemu-system-i386

CFLAGS  := -m32 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-pie -fno-stack-protector
ASFLAGS := --32
LDFLAGS := -m elf_i386 -T linker.ld

BUILD_DIR := build
ISO_DIR   := iso

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
BOOT_OBJ   := $(BUILD_DIR)/boot.o
KERNEL_SRCS := $(wildcard kernel/*.c)
KERNEL_OBJS := $(patsubst kernel/%.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))
ISO_IMAGE  := $(BUILD_DIR)/SodaOS.iso
BUILD_STAMP := $(BUILD_DIR)/.dir

.PHONY: all build iso run run-gtk run-sdl run-curses run-debug clean

all: build

build: $(KERNEL_ELF)

$(BUILD_STAMP):
	mkdir -p $(BUILD_DIR)
	touch $(BUILD_STAMP)

$(BOOT_OBJ): boot/boot.s | $(BUILD_STAMP)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: kernel/%.c | $(BUILD_STAMP)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(BOOT_OBJ) $(KERNEL_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ) $(KERNEL_OBJS)
	$(GRUBFILE) --is-x86-multiboot $@
	cp $@ $(ISO_DIR)/boot/kernel.elf

iso: build
	$(GRUBMK) -o $(ISO_IMAGE) $(ISO_DIR)

run: run-sdl

run-gtk: iso
	$(QEMU) \
		-boot d \
		-drive format=raw,media=cdrom,file=$(ISO_IMAGE) \
		-vga std \
		-display gtk,gl=off \
		-m 256 \
		-no-reboot \
		-no-shutdown

run-sdl: iso
	$(QEMU) \
		-boot d \
		-drive format=raw,media=cdrom,file=$(ISO_IMAGE) \
		-vga std \
		-display sdl \
		-m 256 \
		-no-reboot \
		-no-shutdown

run-curses: iso
	$(QEMU) \
		-boot d \
		-drive format=raw,media=cdrom,file=$(ISO_IMAGE) \
		-display curses \
		-m 256 \
		-no-reboot \
		-no-shutdown

run-debug: iso
	$(QEMU) \
		-boot d \
		-drive format=raw,media=cdrom,file=$(ISO_IMAGE) \
		-vga std \
		-display none \
		-serial stdio \
		-monitor none \
		-m 256 \
		-no-reboot \
		-no-shutdown

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(ISO_DIR)/boot/kernel.elf
