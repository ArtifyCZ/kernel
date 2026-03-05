include ../../common.mk

RUST_TARGET := $(ARCH)-unknown-none

CARGOFLAGS += --target $(RUST_TARGET)
CARGOFLAGS += --profile dev

kernel_dir := $(realpath $(CURDIR))

.PHONY: $(KERNEL_CORE_MODULE__LIB_KERNEL_CORE_A)
$(KERNEL_CORE_MODULE__LIB_KERNEL_CORE_A): $(DEPS)
	cargo build $(CARGOFLAGS)
	mkdir -p $(dir $(KERNEL_CORE_MODULE__LIB_KERNEL_CORE_A))
	cp -v $(kernel_dir)/target/$(RUST_TARGET)/debug/libkernel_core.a $(KERNEL_CORE_MODULE__LIB_KERNEL_CORE_A)

$(DEPS): $(DEPS)/limine-protocol $(DEPS)/freestnd-c-hdrs


$(DEPS)/limine-protocol: repo = https://github.com/limine-bootloader/limine-protocol.git
$(DEPS)/limine-protocol: commit = 42e836e30242c2c14f889fd76c6f9a57b0c18ec2

$(DEPS)/freestnd-c-hdrs: repo = https://github.com/osdev0/freestnd-c-hdrs-0bsd.git
$(DEPS)/freestnd-c-hdrs: commit = 5e4e9e70278fe89ea328d359a58aff4f4a94b165

$(DEPS)/%:
	rm -rf $@
	git clone $(repo) $@
	git -C $@ -c advice.detachedHead=false checkout $(commit)

clean::
	cd $(kernel_dir) && cargo clean
