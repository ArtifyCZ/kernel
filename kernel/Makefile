RUST_TARGET := $(ARCH)-unknown-none

CARGOFLAGS += --target $(RUST_TARGET)
CARGOFLAGS += --profile dev

kernel_dir := $(call get_current_dir)

.PHONY: $(BUILD)/libkernel.$(ARCH).a
$(BUILD)/libkernel.$(ARCH).a:
	cd $(kernel_dir) && cargo build $(CARGOFLAGS)
	cp -v $(kernel_dir)/target/$(RUST_TARGET)/debug/libkernel.a $(BUILD)/libkernel.$(ARCH).a

clean::
	cd $(kernel_dir) && cargo clean
