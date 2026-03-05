load("@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl", "feature", "flag_group", "flag_set", "tool_path")
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load("@rules_cc//cc:defs.bzl", "cc_common")

all_compile_actions = [
    ACTION_NAMES.c_compile,
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.assemble,
    ACTION_NAMES.preprocess_assemble,
]

def _impl(ctx):
    def get_path(f):
        return f.path

    tool_paths = [
        tool_path(name = "gcc",     path = get_path(ctx.file.gcc)),
        tool_path(name = "ld",      path = get_path(ctx.file.ld)),
        tool_path(name = "ar",      path = get_path(ctx.file.ar)),
        tool_path(name = "nm",      path = get_path(ctx.file.nm)),
        tool_path(name = "objcopy", path = get_path(ctx.file.objcopy)),
        tool_path(name = "objdump", path = get_path(ctx.file.objdump)),
        tool_path(name = "strip",   path = get_path(ctx.file.strip)),
        tool_path(name = "cpp",     path = get_path(ctx.file.gcc)),
    ]

    # Mandatory flags for a freestanding kernel
    kernel_flags = [
        "-Wall",
        "-Wextra",
        "-std=gnu11",
        "-nostdinc",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fno-stack-check",
        "-mgeneral-regs-only",
        "-mno-red-zone",
        "-fno-lto",
        "-fno-PIC",
        "-ffunction-sections",
        "-fdata-sections",
    ]

    # Architecture specific (e.g., x86_64)
    if ctx.attr.cpu == "x86_64":
        kernel_flags += ["-target", "x86_64-none-elf"]
        kernel_flags += ["-D__x86_64__"]
    elif ctx.attr.cpu == "aarch64":
        kernel_flags += ["-target", "aarch64-none-elf"]
        kernel_flags += ["-D__aarch64__"]

    features = [
        feature(
            name = "kernel_compile_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_compile_actions,
                    flag_groups = [flag_group(flags = kernel_flags)],
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        toolchain_identifier = "kernel-" + ctx.attr.cpu + "-toolchain",
        host_system_name = "local",
        target_system_name = "freestanding",
        target_cpu = ctx.attr.cpu,
        target_libc = "none",
        compiler = "clang",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {
        "cpu": attr.string(mandatory = True),
        "gcc": attr.label(allow_single_file = True),
        "ld": attr.label(allow_single_file = True),
        "ar": attr.label(allow_single_file = True),
        "nm": attr.label(allow_single_file = True),
        "objcopy": attr.label(allow_single_file = True),
        "objdump": attr.label(allow_single_file = True),
        "strip": attr.label(allow_single_file = True),
    },
)
