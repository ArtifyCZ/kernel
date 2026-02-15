# Check if the architecture is supported.
ifeq ($(filter $(ARCH),aarch64 x86_64),)
    $(error Architecture $(ARCH) not supported)
endif

init_dir := $(realpath $(call get_current_dir))
init_rel_dir := $(call relative_path_from_srctree,$(init_dir))
init_src_dir := $(init_dir)/src
init_obj_build_dir := $(BUILD)/$(call relative_path_from_srctree,$(init_dir))/objs_$(ARCH)

INIT_SRC_FILES :=
INIT_SRC_FILES += $(wildcard $(init_src_dir)/*.c)

INIT_OBJS :=
INIT_OBJS += $(patsubst $(init_src_dir)/%.c,$(init_obj_build_dir)/%.$(ARCH).o,$(INIT_SRC_FILES))


INCLUDE_HEADERS := $(DEPS)/freestnd-c-hdrs/include


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
	-mgeneral-regs-only \
	-mno-red-zone \
	-fno-lto \
	-fno-PIC \
	-ffunction-sections \
	-fdata-sections


$(init_obj_build_dir)/%.$(ARCH).o: $(init_src_dir)/%.c $(BUILD)
	(mkdir -p $(dir $@) || true)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/init.$(ARCH).elf: $(BUILD) $(INIT_OBJS)
	$(LD) -Ttext 0x400000 $(INIT_OBJS) -o $(BUILD)/init.$(ARCH).elf
