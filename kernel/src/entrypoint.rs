use crate::main;

#[unsafe(no_mangle)]
pub unsafe extern "C" fn kernel_main() {
    main();
}
