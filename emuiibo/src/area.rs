use nx::result::*;
use nx::fs;
use nx::ipc::sf::nfp;
use alloc::string::String;
use crate::amiibo;
use crate::fsext;

pub struct ApplicationArea {
    area_file: String
}

impl ApplicationArea {
    pub fn new() -> Self {
        Self { area_file: String::new() }
    }

    pub fn from(virtual_amiibo: &amiibo::VirtualAmiibo, access_id: nfp::AccessId) -> Self {
        let areas_dir = format!("{}/areas", virtual_amiibo.path);
        let _ = fs::create_directory(areas_dir.clone());
        Self { area_file: format!("{}/0x{:08X}.bin", areas_dir, access_id) }
    }

    pub fn is_valid(&self) -> bool {
        !self.area_file.is_empty()
    }

    pub fn exists(&self) -> bool {
        if self.is_valid() {
            fsext::exists_file(self.area_file.clone())
        }
        else {
            false
        }
    }

    pub fn create(&self, data: *const u8, data_size: usize, recreate: bool) -> Result<()> {
        if recreate {
            let _ = fs::delete_file(self.area_file.clone());
        }
        // Write already creates the file if it doesn't exist
        self.write(data, data_size)
    }

    pub fn write(&self, data: *const u8, data_size: usize) -> Result<()> {
        let mut file = fs::open_file(self.area_file.clone(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
        file.write(data, data_size)?;
        Ok(())
    }

    pub fn read(&self, data: *mut u8, data_size: usize) -> Result<()> {
        let mut file = fs::open_file(self.area_file.clone(), fs::FileOpenOption::Read())?;
        file.read(data, data_size)?;
        Ok(())
    }

    pub fn get_size(&self) -> Result<usize> {
        let mut file = fs::open_file(self.area_file.clone(), fs::FileOpenOption::Read())?;
        file.get_size()
    }
}