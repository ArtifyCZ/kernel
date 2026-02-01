default: help

BUILD=build


$(BUILD):
	mkdir -p $(BUILD)


KERNEL_OBJS=

KERNEL_OBJS += $(BUILD)/multiboot_header.o
KERNEL_OBJS += $(BUILD)/multiboot_entry.o
KERNEL_OBJS += $(BUILD)/kernel.o


$(BUILD)/%.o: %.asm $(BUILD)
	nasm -felf32 $< -o $@

$(BUILD)/%.o: %.c $(BUILD)
	clang -target i386-linux-gnu -ffreestanding -Wall -Wextra -c $< -o $@


$(BUILD)/kernel.elf: $(KERNEL_OBJS) $(BUILD)
	ld -m elf_i386 -T kernel.ld -o $(BUILD)/kernel.elf $(KERNEL_OBJS)


$(BUILD)/kernel.iso: $(BUILD)/kernel.elf $(BUILD)
	# pack iso file
	mkdir -p $(BUILD)/isofiles/boot/grub
	cp bootloader/grub-0.97-binaries/iso9660_stage1_5 $(BUILD)/isofiles/boot/grub/stage1
	cp bootloader/grub-0.97-binaries/stage2 $(BUILD)/isofiles/boot/grub/stage2
	cp $(BUILD)/kernel.elf $(BUILD)/isofiles/boot/kernel.elf
	cp bootloader/menu.lst $(BUILD)/isofiles/boot/grub/
	cp bootloader/data_file $(BUILD)/isofiles/boot/
	xorriso -outdev $(BUILD)/kernel.iso -blank as_needed -map $(BUILD)/isofiles / -boot_image grub bin_path=/boot/grub/stage1


.PHONY: qemu qemu-iso
## Run kernel directly in QEMU (no bootloader included)
qemu: $(BUILD)/kernel.elf
	qemu-system-i386 -kernel $(BUILD)/kernel.elf
	#qemu-system-i386 -boot d -cdrom $(BUILD)/kernel.iso

## Run kernel iso in QEMU (GRUB included)
qemu-iso: $(BUILD)/kernel.iso
	qemu-system-i386 -cdrom $(BUILD)/kernel.iso


## Removes all local artifacts
clean:
	rm -rf build/

.PHONY: help
## This help screen
help:
	@printf "Available targets:\n\n"
	@awk '/^[a-zA-Z\-_0-9%:\\]+/ { \
		helpMessage = match(lastLine, /^## (.*)/); \
		if (helpMessage) { \
		helpCommand = $$1; \
		helpMessage = substr(lastLine, RSTART + 3, RLENGTH); \
	gsub("\\\\", "", helpCommand); \
	gsub(":+$$", "", helpCommand); \
		printf "  \x1b[32;01m%-35s\x1b[0m %s\n", helpCommand, helpMessage; \
		} \
	} \
	{ lastLine = $$0 }' $(MAKEFILE_LIST) | sort -u
	@printf "\n"
