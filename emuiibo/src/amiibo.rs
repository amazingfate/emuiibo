use nx::result::*;
use serde::{Serialize, Deserialize};
use alloc::string::String;
use alloc::vec::Vec;
use nx::fs;
use nx::util;
use nx::rand;
use nx::rand::RandomGenerator;
use nx::ipc::cmif::sf::mii;
use nx::ipc::cmif::sf::nfp;

use crate::fsext;
use crate::miiext;

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct VirtualAmiiboUuidInfo {
    use_random_uuid: bool,
    uuid: [u8; 10]
}

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct VirtualAmiiboData {
    uuid_info: VirtualAmiiboUuidInfo,
    name: util::CString<41>,
    first_write_date: nfp::Date,
    last_write_date: nfp::Date,
    mii_charinfo: mii::CharInfo
}

#[derive(Serialize, Deserialize, Debug)]
pub struct VirtualAmiiboId {
    pub game_character_id: u16,
    pub character_variant: u8,
    pub figure_type: u8,
    pub model_number: u16,
    pub series: u8
}

impl VirtualAmiiboId {
    pub const fn empty() -> Self {
        Self { character_variant: 0, figure_type: 0, game_character_id: 0, model_number: 0, series: 0 }
    }
}

#[derive(Serialize, Deserialize, Debug)]
pub struct VirtualAmiiboDate {
    pub y: u16,
    pub m: u8,
    pub d: u8
}

impl VirtualAmiiboDate {
    pub const fn empty() -> Self {
        Self { y: 0, m: 0, d: 0 }
    }

    pub const fn to_date(&self) -> nfp::Date {
        nfp::Date { year: self.y, month: self.m, day: self.d }
    }
}

#[derive(Serialize, Deserialize, Debug)]
pub struct VirtualAmiiboInfo {
    first_write_date: VirtualAmiiboDate,
    id: VirtualAmiiboId,
    last_write_date: VirtualAmiiboDate,
    mii_charinfo_file: String,
    name: String,
    uuid: Option<Vec<u8>>,
    version: u16,
    write_counter: u16
}

impl VirtualAmiiboInfo {
    pub const fn empty() -> Self {
        Self {
            first_write_date: VirtualAmiiboDate::empty(),
            id: VirtualAmiiboId::empty(),
            last_write_date: VirtualAmiiboDate::empty(),
            mii_charinfo_file: String::new(),
            name: String::new(),
            uuid: None,
            version: 0,
            write_counter: 0
        }
    }
}

// No easier way of having a constant way of creating the struct below :P

const EMPTY_MII_CHARINFO: mii::CharInfo = mii::CharInfo {
    id: util::Uuid {
        uuid: [0; 0x10]
    },
    name: util::CString16::new(),
    unk_1: 0,
    mii_color: 0,
    mii_sex: 0,
    mii_height: 0,
    mii_width: 0,
    unk_2: [0; 2],
    mii_face_shape: 0,
    mii_face_color: 0,
    mii_wrinkles_style: 0,
    mii_makeup_style: 0,
    mii_hair_style: 0,
    mii_hair_color: 0,
    mii_has_hair_flipped: 0,
    mii_eye_style: 0,
    mii_eye_color: 0,
    mii_eye_size: 0,
    mii_eye_thickness: 0,
    mii_eye_angle: 0,
    mii_eye_pos_x: 0,
    mii_eye_pos_y: 0,
    mii_eyebrow_style: 0,
    mii_eyebrow_color: 0,
    mii_eyebrow_size: 0,
    mii_eyebrow_thickness: 0,
    mii_eyebrow_angle: 0,
    mii_eyebrow_pos_x: 0,
    mii_eyebrow_pos_y: 0,
    mii_nose_style: 0,
    mii_nose_size: 0,
    mii_nose_pos: 0,
    mii_mouth_style: 0,
    mii_mouth_color: 0,
    mii_mouth_size: 0,
    mii_mouth_thickness: 0,
    mii_mouth_pos: 0,
    mii_facial_hair_color: 0,
    mii_beard_style: 0,
    mii_mustache_style: 0,
    mii_mustache_size: 0,
    mii_mustache_pos: 0,
    mii_glasses_style: 0,
    mii_glasses_color: 0,
    mii_glasses_size: 0,
    mii_glasses_pos: 0,
    mii_has_mole: 0,
    mii_mole_size: 0,
    mii_mole_pos_x: 0,
    mii_mole_pos_y: 0,
    unk_3: 0
};

const DEFAULT_MII_NAME: &'static str = "emuiibo";

pub struct VirtualAmiibo {
    pub info: VirtualAmiiboInfo,
    pub mii_charinfo: mii::CharInfo,
    pub path: String
}

impl VirtualAmiibo {
    pub const fn empty() -> Self {
        // Can't use Default with charinfo here as the function MUST be const - waiting for const traits...
        Self { info: VirtualAmiiboInfo::empty(), mii_charinfo: EMPTY_MII_CHARINFO, path: String::new() }
    }

    pub fn new(info: VirtualAmiiboInfo, path: String) -> Result<Self> {
        let mut amiibo = Self { info: info, mii_charinfo: Default::default(), path: path };
        amiibo.mii_charinfo = amiibo.load_mii_charinfo()?;
        Ok(amiibo)
    }

    pub fn is_valid(&self) -> bool {
        !self.path.is_empty()
    }

    pub fn load_mii_charinfo(&self) -> Result<mii::CharInfo> {
        let mii_charinfo_path = format!("{}/{}", self.path, self.info.mii_charinfo_file);
        if fsext::exists_file(mii_charinfo_path.clone()) {
            let mut mii_charinfo_file = fs::open_file(mii_charinfo_path, fs::FileOpenOption::Read())?;
            Ok(mii_charinfo_file.read_val()?)
        }
        else {
            let mut random_mii = miiext::generate_random_mii()?;
            random_mii.name.set_str(DEFAULT_MII_NAME)?;
            
            let mut mii_charinfo_file = fs::open_file(mii_charinfo_path, fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
            mii_charinfo_file.write_val(random_mii)?;
            Ok(random_mii)
        }
    }

    pub fn produce_data(&self) -> Result<VirtualAmiiboData> {
        let mut data: VirtualAmiiboData = Default::default();
        match self.info.uuid.as_ref() {
            Some(uuid) => {
                for i in 0..data.uuid_info.uuid.len() {
                    data.uuid_info.uuid[i] = uuid[i];
                }
            },
            None => data.uuid_info.use_random_uuid = true
        };

        data.name.set_string(self.info.name.clone())?;
        data.first_write_date = self.info.first_write_date.to_date();
        data.last_write_date = self.info.last_write_date.to_date();
        data.mii_charinfo = self.mii_charinfo;
        Ok(data)
    }

    pub fn produce_tag_info(&self) -> Result<nfp::TagInfo> {
        let mut tag_info: nfp::TagInfo = unsafe { core::mem::zeroed() };
        tag_info.uuid_length = tag_info.uuid.len() as u8;
        unsafe {
            match self.info.uuid.as_ref() {
                Some(uuid) => core::ptr::copy(uuid.as_ptr(), tag_info.uuid.as_mut_ptr(), tag_info.uuid.len()),
                None => {
                    let mut rng = rand::SplCsrngGenerator::new()?;
                    rng.random_bytes(tag_info.uuid.as_mut_ptr(), tag_info.uuid.len())?;
                }
            };
        }
        tag_info.tag_type = u32::max_value();
        tag_info.protocol = u32::max_value();
        Ok(tag_info)
    }

    pub fn produce_register_info(&self) -> Result<nfp::RegisterInfo> {
        let mut register_info: nfp::RegisterInfo = unsafe { core::mem::zeroed() };
        register_info.mii_charinfo = self.mii_charinfo;
        register_info.name.set_string(self.info.name.clone())?;
        register_info.first_write_date = self.info.first_write_date.to_date();
        Ok(register_info)
    }

    pub fn produce_common_info(&self) -> Result<nfp::CommonInfo> {
        let mut common_info: nfp::CommonInfo = unsafe { core::mem::zeroed() };
        common_info.last_write_date = self.info.first_write_date.to_date();
        common_info.application_area_size = 0xD8;
        common_info.version = self.info.version;
        common_info.write_counter = self.info.write_counter;
        Ok(common_info)
    }

    pub fn produce_model_info(&self) -> Result<nfp::ModelInfo> {
        let mut model_info: nfp::ModelInfo = unsafe { core::mem::zeroed() };
        model_info.game_character_id = self.info.id.game_character_id;
        model_info.character_variant = self.info.id.character_variant;
        model_info.figure_type = self.info.id.figure_type;
        model_info.model_number = self.info.id.model_number;
        model_info.series = self.info.id.series;
        Ok(model_info)
    }

    pub fn save(&self) -> Result<()> {
        if let Ok(data) = serde_json::to_vec_pretty(&self.info) {
            let amiibo_json_file = format!("{}/amiibo.json", self.path);
            let _ = fs::delete_file(amiibo_json_file.clone());
            let mut amiibo_json = fs::open_file(amiibo_json_file.clone(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
            amiibo_json.write(data.as_ptr(), data.len())?;
            return Ok(());
        }
        Err(ResultCode::new(0xBEBE))
    }

    pub fn notify_written(&mut self) -> Result<()> {
        if self.info.write_counter < 0xFFFF {
            self.info.write_counter += 1;
        }
        self.save()
    }
}

pub fn try_load_virtual_amiibo(path: String) -> Result<VirtualAmiibo> {
    let amiibo_flag_file = format!("{}/amiibo.flag", path);
    result_return_unless!(fsext::exists_file(amiibo_flag_file), 0xBEBE);

    let amiibo_json_file = format!("{}/amiibo.json", path);
    result_return_unless!(fsext::exists_file(amiibo_json_file.clone()), 0xBEBE);

    let mut amiibo_json = fs::open_file(amiibo_json_file, fs::FileOpenOption::Read())?;
    let mut amiibo_json_data: Vec<u8> = vec![0; amiibo_json.get_size()?];
    amiibo_json.read(amiibo_json_data.as_mut_ptr(), amiibo_json_data.len())?;
    if let Ok(amiibo_json_str) = core::str::from_utf8(amiibo_json_data.as_slice()) {
        if let Ok(virtual_amiibo_info) = serde_json::from_str::<VirtualAmiiboInfo>(amiibo_json_str) {
            return VirtualAmiibo::new(virtual_amiibo_info, path.clone());
        }
    }
    Err(ResultCode::new(0xBEBE))
}