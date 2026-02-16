use std::env;
use std::fs::create_dir_all;
use std::path::PathBuf;

fn main() {
    let target = env::var("TARGET").unwrap();

    let headers = [
        ("../platform/include/drivers/keyboard.h", "drivers/keyboard.rs"),
        ("../platform/include/drivers/serial.h", "drivers/serial.rs"),
        ("../platform/include/elf.h", "elf.rs"),
        ("../platform/include/modules.h", "modules.rs"),
        ("../platform/include/physical_memory_manager.h", "physical_memory_manager.rs"),
        ("../platform/include/scheduler.h", "scheduler.rs"),
        ("../platform/include/terminal.h", "terminal.rs"),
        ("../platform/include/thread.h", "thread.rs"),
        ("../platform/include/ticker.h", "ticker.rs"),
        ("../platform/include/timer.h", "timer.rs"),
        ("../platform/include/virtual_address_allocator.h", "virtual_address_allocator.rs"),
        ("../platform/include/virtual_memory_manager.h", "virtual_memory_manager.rs"),
    ];

    for (header, module) in headers {
        // The bindgen::Builder is the main entry point
        // to bindgen, and lets you build up options for
        // the resulting bindings.
        let bindings = bindgen::Builder::default()
            // The input header we would like to generate
            // bindings for.
            .use_core()
            .header(header)
            // Tell cargo to invalidate the built crate whenever any of the
            // included header files changed.
            .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
            // Finish the builder and generate the bindings.
            .clang_arg(format!("--target={}", target))
            .clang_arg("-nostdinc")
            .clang_arg("-std=gnu11")
            .clang_arg("-ffreestanding")
            .clang_arg("-fno-PIC")
            .clang_arg("-ffunction-sections")
            .clang_arg("-fdata-sections")
            .clang_arg("-I./../dependencies/freestnd-c-hdrs/include")
            .clang_arg("-I./../dependencies/limine-protocol/include")
            .generate()
            // Unwrap the Result and panic on failure.
            .expect("Unable to generate bindings");

        // Write the bindings to the $OUT_DIR/bindings.rs file.
        let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
        let module_path = out_path.join("bindings").join(module);
        create_dir_all(module_path.parent().unwrap()).unwrap();
        bindings
            .write_to_file(module_path)
            .expect("Couldn't write bindings!");
    }
}
