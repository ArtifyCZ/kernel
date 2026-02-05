BUILD=build
DEPS=dependencies




# Check if the architecture is supported.
ifeq ($(filter $(ARCH),aarch64 x86_64),)
    $(error Architecture $(ARCH) not supported)
endif


default: help

$(BUILD):
	mkdir -p $(BUILD)

all: $(BUILD)/libplatform.a


PLATFORM_SRC_FILES :=

PLATFORM_SRC_FILES += $(wildcard src/*.c) $(wildcard src/**/*.c)

PLATFORM_ASM_FILES :=
PLATFORM_ASM_FILES += $(wildcard src/*.$(ARCH).asm) $(wildcard src/**/*.$(ARCH).asm)
PLATFORM_ASM_FILES += $(wildcard src/*.$(ARCH).S) $(wildcard src/**/*.$(ARCH).S)

PLATFORM_OBJS=
PLATFORM_OBJS += $(patsubst %.c,$(BUILD)/%.o,$(PLATFORM_SRC_FILES))
PLATFORM_OBJS += $(patsubst %.asm,$(BUILD)/%.o,$(PLATFORM_ASM_FILES))
PLATFORM_OBJS += $(patsubst %.S,$(BUILD)/%.o,$(PLATFORM_ASM_FILES))


INCLUDE_HEADERS=

INCLUDE_HEADERS += $(DEPS)/limine-protocol/include
INCLUDE_HEADERS += $(DEPS)/freestnd-c-hdrs/include


CFLAGS :=
CFLAGS += $(addprefix -I ,$(INCLUDE_HEADERS))
CFLAGS += -g -O0 -pipe
CFLAGS += -target $(ARCH)-none-elf
CFLAGS += -Wall \
	-Wextra \
	-std=gnu11 \
	-nostdinc \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-mno-red-zone \
	-fno-lto \
	-fno-PIC \
	-ffunction-sections \
	-fdata-sections


LDFLAGS :=
LDFLAGS += -m aarch64elf
LDFLAGS += -nostdlib \
	-static \
	--gc-sections


ifeq ($(ARCH),x86_64)
	NASMFLAGS += -f elf64
else ifeq ($(ARCH),aarch64)
else
$(error Compilation of assembly not configured for architecture $(ARCH) yet)
endif


$(BUILD)/%.x86_64.o: %.x86_64.asm $(BUILD)
	(mkdir -p $(dir $@) || true)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD)/%.aarch64.o: %.aarch64.S $(BUILD)
	(mkdir -p $(dir $@) || true)
	$(AARCH64_ELF_AS) $< -o $@

$(BUILD)/%.o: %.c $(BUILD)
	(mkdir -p $(dir $@) || true)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/libplatform.a: $(BUILD) $(DEPS) $(PLATFORM_OBJS)
	(rm $(BUILD)/libplatform.a || true)
	ar rcs $(BUILD)/libplatform.a $(PLATFORM_OBJS)


$(DEPS): $(DEPS)/limine-protocol $(DEPS)/freestnd-c-hdrs


$(DEPS)/limine-protocol: repo = https://codeberg.org/Limine/limine-protocol.git
$(DEPS)/limine-protocol: commit = 42e836e30242c2c14f889fd76c6f9a57b0c18ec2

$(DEPS)/freestnd-c-hdrs: repo = https://codeberg.org/OSDev/freestnd-c-hdrs-0bsd.git
$(DEPS)/freestnd-c-hdrs: commit = 097259a899d30f0a4b7a694de2de5fdda942e923


$(DEPS)/%:
	rm -rf $@
	git clone $(repo) $@
	git -C $@ -c advice.detachedHead=false checkout $(commit)


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
