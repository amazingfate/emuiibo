use nx::result::*;
use nx::results;
use nx::fs;
use alloc::string::String;

pub fn exists_file(path: String) -> bool {
    match fs::create_file(path.clone(), 0, fs::FileAttribute::None()) {
        Ok(()) => {
            let _ = fs::delete_file(path.clone());
            false
        },
        Err(rc) => {
            if results::fs::ResultPathAlreadyExists::matches(rc) {
                true
            }
            else {
                false
            }
        }
    }
}

pub fn recreate_directory(path: String) {
    let _ = fs::delete_directory(path.clone());
    let _ = fs::create_directory(path.clone());
}

pub const BASE_DIR: &'static str = "sdmc:/emuiibo";
pub const VIRTUAL_AMIIBO_DIR: &'static str = "sdmc:/emuiibo/amiibo";
pub const EXPORTED_MIIS_DIR: &'static str = "sdmc:/emuiibo/miis";

pub fn ensure_directories() {
    let _ = fs::create_directory(String::from(BASE_DIR));
    let _ = fs::create_directory(String::from(VIRTUAL_AMIIBO_DIR));
    recreate_directory(String::from(EXPORTED_MIIS_DIR));
}