default: help

BUILD=build
DEPS=dependencies

# Toolchain for building the 'limine' executable for the host.
HOST_CC := clang
HOST_CFLAGS := -g -O2 -pipe
HOST_CPPFLAGS :=
HOST_LDFLAGS :=
HOST_LIBS :=


$(BUILD):
	mkdir -p $(BUILD)


all: $(BUILD)/kernel.iso


KERNEL_SRC_FILES :=

KERNEL_SRC_FILES += $(wildcard *.c)

KERNEL_NASM_FILES :=

KERNEL_NASM_FILES += $(wildcard *.asm)

KERNEL_OBJS=
KERNEL_OBJS += $(patsubst %.c,$(BUILD)/%.o,$(KERNEL_SRC_FILES))
KERNEL_OBJS += $(patsubst %.asm,$(BUILD)/%.o,$(KERNEL_NASM_FILES))


INCLUDE_HEADERS=

INCLUDE_HEADERS += $(DEPS)/limine-protocol/include
INCLUDE_HEADERS += $(DEPS)/freestnd-c-hdrs/include


CFLAGS :=
CFLAGS += $(addprefix -I ,$(INCLUDE_HEADERS))
CFLAGS += -g -O0 -pipe -target x86_64-linux-gnu
CFLAGS += -Wall \
	-Wextra \
	-std=gnu11 \
	-nostdinc \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-lto \
	-fno-PIC \
	-ffunction-sections \
	-fdata-sections


LDFLAGS :=
LDFLAGS += -m elf_x86_64
LDFLAGS += -nostdlib \
	-static \
	-z max-page-size=0x1000 \
	--gc-sections
LDFLAGS += -T kernel.ld


$(BUILD)/%.o: %.asm $(BUILD)
	nasm -felf64 $< -o $@

$(BUILD)/%.o: %.c $(BUILD)
	clang $(CFLAGS) -c $< -o $@


$(BUILD)/kernel.elf: $(DEPS) $(KERNEL_OBJS) $(BUILD)
	ld $(LDFLAGS) -o $(BUILD)/kernel.elf $(KERNEL_OBJS)


$(BUILD)/kernel.iso: $(BUILD)/kernel.elf $(BUILD)/limine/limine $(BUILD)
	mkdir -p $(BUILD)/isofiles/boot/limine/
	cp -v limine.conf $(BUILD)/isofiles/boot/limine/
	mkdir -p $(BUILD)/isofiles/EFI/BOOT

	cp $(BUILD)/kernel.elf $(BUILD)/isofiles/boot/kernel.elf

	cp -v $(BUILD)/limine/limine-bios.sys $(BUILD)/limine/limine-bios-cd.bin $(BUILD)/limine/limine-uefi-cd.bin $(BUILD)/isofiles/boot/limine/
	cp -v $(BUILD)/limine/BOOTX64.EFI $(BUILD)/isofiles/EFI/BOOT/
	cp -v $(BUILD)/limine/BOOTIA32.EFI $(BUILD)/isofiles/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(BUILD)/isofiles -o $(BUILD)/kernel.iso
	$(BUILD)/limine/limine bios-install $(BUILD)/kernel.iso
	rm -rf $(BUILD)/isofiles

# xorriso -outdev $(BUILD)/kernel.iso -blank as_needed -map $(BUILD)/isofiles / -boot_image grub bin_path=/boot/grub/stage1


$(DEPS): $(DEPS)/limine-protocol $(DEPS)/freestnd-c-hdrs


$(DEPS)/limine-protocol: repo = https://codeberg.org/Limine/limine-protocol.git
$(DEPS)/limine-protocol: commit = 42e836e30242c2c14f889fd76c6f9a57b0c18ec2

$(DEPS)/freestnd-c-hdrs: repo = https://codeberg.org/OSDev/freestnd-c-hdrs-0bsd.git
$(DEPS)/freestnd-c-hdrs: commit = 097259a899d30f0a4b7a694de2de5fdda942e923


$(DEPS)/%:
	rm -rf $@
	git clone $(repo) $@
	git -C $@ -c advice.detachedHead=false checkout $(commit)

$(BUILD)/limine/limine:
	rm -rf $(BUILD)/limine
	git clone https://codeberg.org/Limine/Limine.git $(BUILD)/limine --branch=v10.x-binary --depth=1
	$(MAKE) -C $(BUILD)/limine \
		CC="$(HOST_CC)" \
		CFLAGS="$(HOST_CFLAGS)" \
		CPPFLAGS="$(HOST_CPPFLAGS)" \
		LDFLAGS="$(HOST_LDFLAGS)" \
		LIBS="$(HOST_LIBS)"


.PHONY: qemu qemu-debug
qemu: $(BUILD)/kernel.iso
	qemu-system-x86_64 -serial stdio -cdrom $(BUILD)/kernel.iso

qemu-debug: $(BUILD)/kernel.iso
	qemu-system-x86_64 -serial stdio -s -S -cdrom $(BUILD)/kernel.iso


## Removes all local artifacts
clean:
	rm -rf $(BUILD)/
	rm -rf $(DEPS)/

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
