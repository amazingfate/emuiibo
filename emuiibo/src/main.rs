#![no_std]
#![no_main]

#![feature(const_fn)]

#[macro_use]
extern crate nx;

#[macro_use]
extern crate alloc;

extern crate paste;

use nx::result::*;
use nx::util;
use nx::thread;
use nx::diag::assert;
use nx::diag::log;
use nx::ipc::cmif::server;
use nx::fs;
use core::panic;

mod resultsext;
mod fsext;
mod miiext;
mod ipc;
mod emu;
mod amiibo;
mod area;
mod logger;

const STACK_HEAP_SIZE: usize = 0x4000;
static mut STACK_HEAP: [u8; STACK_HEAP_SIZE] = [0; STACK_HEAP_SIZE];

#[no_mangle]
pub fn initialize_heap(_hbl_heap: util::PointerAndSize) -> util::PointerAndSize {
    unsafe {
        util::PointerAndSize::new(STACK_HEAP.as_mut_ptr(), STACK_HEAP.len())
    }
}

const POINTER_BUF_SIZE: usize = 0x1000;
type Manager = server::ServerManager<POINTER_BUF_SIZE>;

#[no_mangle]
pub fn main() -> Result<()> {
    thread::get_current_thread().name.set_str("emuiibo.Main")?;
    fs::initialize()?;
    fs::mount_sd_card("sdmc")?;
    fsext::ensure_directories();
    miiext::initialize()?;
    miiext::export_miis()?;
    logger::initialize();

    let mut manager = Manager::new()?;
    manager.register_mitm_service_server::<ipc::nfp::UserManager>()?;
    manager.register_service_server::<ipc::emu::EmulationService>()?;
    manager.loop_process()?;

    miiext::finalize();
    fs::finalize();
    Ok(())
}

#[panic_handler]
fn panic_handler(info: &panic::PanicInfo) -> ! {
    util::simple_panic_handler::<log::LmLogger>(info, assert::AssertMode::SvcBreak)
}