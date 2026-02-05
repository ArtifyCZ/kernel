default: help

ARCH=x86_64
BUILD=build


CC := clang
LD := ld.lld
NASM := nasm


$(BUILD):
	mkdir -p $(BUILD)


all: $(BUILD)/kernel.iso


LDFLAGS :=
LDFLAGS += -m elf_$(ARCH)
LDFLAGS += -nostdlib \
	-static \
	-z max-page-size=0x1000 \
	--gc-sections
LDFLAGS += -T kernel.ld
LDFLAGS += -L $(BUILD)
LDFLAGS += -l:libplatform.a
LDFLAGS += -l:libkernel.a

MAKE_PLATFORM := $(MAKE) -C platform
MAKE_PLATFORM += ARCH="$(ARCH)"
MAKE_PLATFORM += ARCH="$(ARCH)"
MAKE_PLATFORM += CC="$(CC)"
MAKE_PLATFORM += NASM="$(NASM)"
MAKE_PLATFORM += LD="$(LD)"

MAKE_KERNEL := $(MAKE) -C kernel
MAKE_KERNEL += ARCH="$(ARCH)"

MAKE_LIMINE := $(MAKE) -C $(BUILD)/limine
MAKE_LIMINE += CC="$(CC)"
MAKE_LIMINE += CFLAGS="-g -O2 -pipe"
MAKE_LIMINE += CPPFLAGS=""
MAKE_LIMINE += LDFLAGS=""
MAKE_LIMINE += LIBS=""

.PHONY: $(BUILD)/libplatform.a
$(BUILD)/libplatform.a: $(BUILD)
	$(MAKE_PLATFORM) all
	cp -v platform/build/libplatform.a $(BUILD)/

.PHONY: $(BUILD)/libkernel.a
$(BUILD)/libkernel.a: $(BUILD)
	$(MAKE_KERNEL) build-dev
	cp -v kernel/target/$(ARCH)-unknown-none/debug/libkernel.a $(BUILD)/


.PHONY: platform/clean
platform/clean:
	$(MAKE_PLATFORM) clean

.PHONY: kernel/clean
kernel/clean:
	$(MAKE_KERNEL) clean

$(BUILD)/kernel.elf: $(BUILD) $(BUILD)/libkernel.a $(BUILD)/libplatform.a
	$(LD) $(LDFLAGS) -o $(BUILD)/kernel.elf


$(BUILD)/kernel.iso: $(BUILD)/kernel.elf $(BUILD)/limine/limine $(BUILD)
	mkdir -p $(BUILD)/isofiles/boot/limine/
	cp -v limine.conf $(BUILD)/isofiles/boot/limine/
	mkdir -p $(BUILD)/isofiles/EFI/BOOT

	cp $(BUILD)/kernel.elf $(BUILD)/isofiles/boot/kernel.elf
	cp Mik_8x16.psf $(BUILD)/isofiles/boot/kernel-font.psf

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


$(BUILD)/limine/limine:
	rm -rf $(BUILD)/limine
	git clone https://codeberg.org/Limine/Limine.git $(BUILD)/limine --branch=v10.x-binary --depth=1
	$(MAKE_LIMINE)


.PHONY: qemu qemu-debug
qemu: $(BUILD)/kernel.iso
	qemu-system-x86_64 -serial stdio -m 256M -cdrom $(BUILD)/kernel.iso

qemu-debug: $(BUILD)/kernel.iso
	qemu-system-x86_64 -serial stdio -m 256M -s -S -cdrom $(BUILD)/kernel.iso


## Removes all local artifacts
clean: kernel/clean platform/clean
	rm -rf $(BUILD)/

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
