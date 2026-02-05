default: help
.SUFFIXES:            # Delete the default suffixes

ARCH := x86_64
BUILD := $(abspath ./build)
DEPS := $(abspath ./dependencies)


CC := clang
LD := ld.lld
NASM := nasm
AARCH64_ELF_AS := aarch64-elf-as

get_current_dir = $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
srctree := $(call get_current_dir)

relative_path_from_srctree = $(1:$(srctree)/%=%)

$(BUILD):
	mkdir -p $(BUILD)


.PHONY: all
all:: $(BUILD)/kernel.$(ARCH).iso


LDFLAGS :=
LDFLAGS += -nostdlib \
	-static \
	-z max-page-size=0x1000 \
	--gc-sections
LDFLAGS += -T kernel.$(ARCH).ld
LDFLAGS += -L $(BUILD)
LDFLAGS += -l:libplatform.$(ARCH).a
LDFLAGS += -l:libkernel.$(ARCH).a


MAKE_LIMINE := $(MAKE) -C $(BUILD)/limine
MAKE_LIMINE += CC="$(CC)"
MAKE_LIMINE += CFLAGS="-g -O2 -pipe"
MAKE_LIMINE += CPPFLAGS=""
MAKE_LIMINE += LDFLAGS=""
MAKE_LIMINE += LIBS=""


include kernel/Makefile
include platform/Makefile


$(BUILD)/kernel.$(ARCH).elf: $(BUILD) $(BUILD)/libkernel.$(ARCH).a $(BUILD)/libplatform.$(ARCH).a
	$(LD) $(LDFLAGS) -o $(BUILD)/kernel.$(ARCH).elf

isofiles_dir := $(BUILD)/isofiles/$(ARCH)

$(BUILD)/kernel.$(ARCH).iso: $(BUILD)/kernel.$(ARCH).elf $(BUILD)/limine/limine $(BUILD)
	mkdir -p $(isofiles_dir)/boot/limine/
	cp -v limine.conf $(isofiles_dir)/boot/limine/
	mkdir -p $(isofiles_dir)/EFI/BOOT

	cp $(BUILD)/kernel.$(ARCH).elf $(isofiles_dir)/boot/kernel.elf
	cp Mik_8x16.psf $(isofiles_dir)/boot/kernel-font.psf

	cp -v $(BUILD)/limine/limine-bios.sys $(BUILD)/limine/limine-bios-cd.bin $(BUILD)/limine/limine-uefi-cd.bin $(isofiles_dir)/boot/limine/
	cp -v $(BUILD)/limine/BOOTX64.EFI $(isofiles_dir)/EFI/BOOT/
	cp -v $(BUILD)/limine/BOOTIA32.EFI $(isofiles_dir)/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(isofiles_dir) -o $(BUILD)/kernel.$(ARCH).iso
	$(BUILD)/limine/limine bios-install $(BUILD)/kernel.$(ARCH).iso
	rm -rf $(BUILD)/isofiles


$(BUILD)/limine/limine:
	rm -rf $(BUILD)/limine
	git clone https://codeberg.org/Limine/Limine.git $(BUILD)/limine --branch=v10.x-binary --depth=1
	$(MAKE_LIMINE)

QEMUFLAGS += -serial stdio -cdrom $(BUILD)/kernel.$(ARCH).iso

.PHONY: qemu qemu-debug
qemu: $(BUILD)/kernel.$(ARCH).iso
	qemu-system-$(ARCH) $(QEMUFLAGS)

qemu-debug: $(BUILD)/kernel.$(ARCH).iso
	qemu-system-$(ARCH) -s -S $(QEMUFLAGS)


## Removes all local artifacts
clean::
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
