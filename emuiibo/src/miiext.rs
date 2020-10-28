use nx::result::*;
use nx::ipc::sf;
use nx::service;
use nx::service::mii;
use nx::service::mii::IDatabaseService;
use nx::service::mii::IStaticService;
use nx::mem;
use nx::fs;
use alloc::vec::Vec;

use crate::fsext;

static mut G_STATIC_SRV: mem::Shared<mii::StaticService> = mem::Shared::empty();
static mut G_DB_SRV: mem::Shared<mii::DatabaseService> = mem::Shared::empty();
static mut G_INIT: bool = false;

pub fn initialize() -> Result<()> {
    unsafe {
        if !G_INIT {
            G_STATIC_SRV = service::new_service_object()?;
            G_DB_SRV = G_STATIC_SRV.get().get_database_service(mii::SpecialKeyCode::Normal)?.to::<mii::DatabaseService>();
            G_INIT = true;
        }
    }
    Ok(())
}

pub fn finalize() {
    unsafe {
        if G_INIT {
            G_DB_SRV.reset();
            G_STATIC_SRV.reset();
            G_INIT = false;
        }
    }
}

pub fn generate_random_mii() -> Result<mii::CharInfo> {
    unsafe {
        G_DB_SRV.get().build_random(mii::Age::All, mii::Gender::All, mii::FaceColor::All)
    }
}

const MII_SOURCE_FLAG: mii::SourceFlag = mii::SourceFlag::Database();

pub fn export_miis() -> Result<()> {
    unsafe {
        if G_INIT {
            let mii_count = G_DB_SRV.get().get_count(MII_SOURCE_FLAG)?;
            let miis: Vec<mii::CharInfo> = vec![Default::default(); mii_count as usize];

            let mii_total = G_DB_SRV.get().get_1(MII_SOURCE_FLAG, sf::Buffer::from_array(&miis))?;
            for i in 0..mii_total {
                let mii = miis[i as usize];

                let mii_dir_path = format!("{}/{}", fsext::EXPORTED_MIIS_DIR, i);
                fs::create_directory(mii_dir_path.clone())?;

                let mii_path = format!("{}/mii-charinfo.bin", mii_dir_path);
                let mut mii_file = fs::open_file(mii_path, fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
                mii_file.write_val(mii)?;

                let mii_name = format!("{}/name.txt", mii_dir_path);
                let mut mii_name_file = fs::open_file(mii_name, fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
                let actual_name = mii.name.get_string()?;
                mii_name_file.write(actual_name.as_ptr(), actual_name.len())?;
            }
        }
    }
    Ok(())
}