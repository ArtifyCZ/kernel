RUST_TARGET := $(ARCH)-unknown-none

CARGOFLAGS :=
CARGOFLAGS += --target $(RUST_TARGET)
CARGOFLAGS += --profile dev

.PHONY: build-dev
build-dev:
	cargo build $(CARGOFLAGS)

.PHONY: clean
clean:
	cargo clean
