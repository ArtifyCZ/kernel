mod bindings {
    include_bindings!("ticker.rs");
}

pub struct Ticker;

impl Ticker {
    pub unsafe fn init() {
        unsafe {
            bindings::ticker_init();
        }
    }
}
