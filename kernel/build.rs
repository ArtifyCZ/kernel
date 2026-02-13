use std::env;
use std::path::PathBuf;

fn main() {
    let target = env::var("TARGET").unwrap();

    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let bindings = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .use_core()
        .header("../platform/include/drivers/serial.h")
        .header("../platform/include/physical_memory_manager.h")
        .header("../platform/include/virtual_memory_manager.h")
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
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("platform_bindings.rs"))
        .expect("Couldn't write bindings!");
}
