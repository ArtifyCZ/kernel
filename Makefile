default: help
.SUFFIXES:            # Delete the default suffixes

include common.mk


.PHONY: all

IMAGE_KERNEL_ISO := $(BUILD)/kernel.$(ARCH).iso
IMAGE_KERNEL_IMG := $(BUILD)/kernel.$(ARCH).img

ifeq ($(ARCH),x86_64)
all:: $$(IMAGE_KERNEL_ISO)

else ifeq ($(ARCH),aarch64)
all:: $(IMAGE_KERNEL_IMG)

endif


.PHONY: $(IMAGE_KERNEL_ISO) $(IMAGE_KERNEL_IMG)
$(BUILD)/kernel.x86_64.iso: ; mkdir -p $(BUILD) && bazel build //:kernel.iso --config x86_64 && rm -f "$@" && cp "$$(bazel cquery //:kernel.iso --config x86_64 --output=files | head -n 1)" "$@"

$(BUILD)/kernel.aarch64.img: ; mkdir -p $(BUILD) && bazel build //:kernel.img --config aarch64 && rm -f "$@" && cp "$$(bazel cquery //:kernel.img --config aarch64 --output=files | head -n 1)" "$@"


QEMU := qemu-system-$(ARCH)

ifeq ($(ARCH),x86_64)
QEMU_IMAGE := $(IMAGE_KERNEL_ISO)

QEMUFLAGS += -cdrom $(QEMU_IMAGE)
#QEMUFLAGS += -d int,cpu_reset
QEMUFLAGS += -serial stdio

else ifeq ($(ARCH),aarch64)
QEMU += -M virt,highmem=on,gic-version=2
QEMU_IMAGE := $(IMAGE_KERNEL_IMG)

QEMUFLAGS += -cpu cortex-a72 -m 2G
QEMUFLAGS += -bios $(QEMU_AARCH64_BIOS)
QEMUFLAGS += -drive file=$(QEMU_IMAGE),if=none,format=raw,id=hd0,readonly=on
QEMUFLAGS += -device virtio-blk-device,drive=hd0
QEMUFLAGS += -d int,mmu,guest_errors -D qemu.log
QEMUFLAGS += -device ramfb # TODO: make it work with virtio-gpu-pci and replace the ramfb
QEMUFLAGS += -device qemu-xhci -device usb-kbd
QEMUFLAGS += -chardev stdio,id=con0 -serial chardev:con0

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
