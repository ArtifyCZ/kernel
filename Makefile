default: help
.SUFFIXES:            # Delete the default suffixes

ARCH := aarch64
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

ifeq ($(ARCH),x86_64)
all:: $(BUILD)/kernel.$(ARCH).iso

else ifeq ($(ARCH),aarch64)
all:: $(BUILD)/kernel.$(ARCH).img

endif


LDFLAGS :=
LDFLAGS += -nostdlib \
	-static \
	-z max-page-size=0x1000 \
	--no-dynamic-linker \
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

.PHONY: $(BUILD)/kernel.x86_64.iso
$(BUILD)/kernel.x86_64.iso: $(BUILD)/kernel.$(ARCH).elf $(BUILD)/limine/limine $(BUILD)
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

.PHONY: $(BUILD)/kernel.aarch64.img
$(BUILD)/kernel.aarch64.img: $(BUILD)/kernel.$(ARCH).elf $(BUILD)/limine/limine $(BUILD)
	(rm -rf $(BUILD)/kernel.aarch64.img || true)
	dd if=/dev/zero of=$@ bs=1M count=64
	mformat -i $@ ::
	mmd -i $@ ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i $@ $(BUILD)/limine/BOOTAA64.EFI ::/EFI/BOOT/
	mcopy -i $@ $(srctree)/limine.conf ::/boot/limine/
	mcopy -i $@ $(BUILD)/kernel.$(ARCH).elf ::/boot/kernel.elf
	mcopy -i $@ $(srctree)/Mik_8x16.psf ::/boot/kernel-font.psf


$(BUILD)/limine/limine:
	rm -rf $(BUILD)/limine
	git clone https://codeberg.org/Limine/Limine.git $(BUILD)/limine --branch=v10.x-binary --depth=1
	$(MAKE_LIMINE)

$(BUILD)/raspi4b-uefi-firmware:
	(rm -rf $(BUILD)/raspi4b-uefi-firmware || true)
	mkdir -p $(BUILD)/raspi4b-uefi-firmware
	curl -L "https://github.com/pftf/RPi4/releases/download/v1.50/RPi4_UEFI_Firmware_v1.50.zip" \
		-o "$(BUILD)/raspi4b-uefi-firmware/firmware.zip"
	unzip $(BUILD)/raspi4b-uefi-firmware/firmware.zip -d $(BUILD)/raspi4b-uefi-firmware


QEMU := qemu-system-$(ARCH)

QEMUFLAGS += -serial stdio


ifeq ($(ARCH),x86_64)
QEMU_IMAGE := $(BUILD)/kernel.$(ARCH).iso

QEMUFLAGS += -cdrom $(QEMU_IMAGE)

else ifeq ($(ARCH),aarch64)
QEMU += -M virt,highmem=on
QEMU_IMAGE := $(BUILD)/kernel.aarch64.img

QEMUFLAGS += -cpu cortex-a72 -m 2G
QEMUFLAGS += -bios /opt/homebrew/opt/qemu/share/qemu/edk2-aarch64-code.fd
QEMUFLAGS += -drive file=$(QEMU_IMAGE),if=none,format=raw,id=hd0,readonly=on
QEMUFLAGS += -device virtio-blk-device,drive=hd0
QEMUFLAGS += -device ramfb # TODO: make it work with virtio-gpu-pci and replace the ramfb
QEMUFLAGS += -device qemu-xhci -device usb-kbd

else
$(error Architecture $(ARCH) not configured for Qemu)
endif


.PHONY: qemu qemu-debug

qemu: $(QEMU_IMAGE)
	$(QEMU) $(QEMUFLAGS)

qemu-debug: $(QEMU_IMAGE)
	$(QEMU) -s -S $(QEMUFLAGS)


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
