use nx::result::*;
use nx::ipc::cmif::sf;
use nx::ipc::cmif::server;
use alloc::string::String;

use crate::resultsext;
use crate::emu;
use crate::amiibo;
use crate::fsext;

pub trait IEmulationService {
    ipc_cmif_interface_define_command!(get_version: () => (version: emu::Version));
    ipc_cmif_interface_define_command!(get_virtual_amiibo_directory: (out_path: sf::OutMapAliasBuffer) => ());
    ipc_cmif_interface_define_command!(get_emulation_status: () => (status: emu::EmulationStatus));
    ipc_cmif_interface_define_command!(set_emulation_status: (status: emu::EmulationStatus) => ());
    ipc_cmif_interface_define_command!(get_active_virtual_amiibo: (out_path: sf::OutMapAliasBuffer) => (virtual_amiibo: amiibo::VirtualAmiiboData));
    ipc_cmif_interface_define_command!(set_active_virtual_amiibo: (path: sf::InMapAliasBuffer) => ());
    ipc_cmif_interface_define_command!(reset_active_virtual_amiibo: () => ());
    ipc_cmif_interface_define_command!(get_active_virtual_amiibo_status: () => (status: emu::VirtualAmiiboStatus));
    ipc_cmif_interface_define_command!(set_active_virtual_amiibo_status: (status: emu::VirtualAmiiboStatus) => ());
    ipc_cmif_interface_define_command!(is_application_id_intercepted: (application_id: u64) => (is_intercepted: bool));
    ipc_cmif_interface_define_command!(try_parse_virtual_amiibo: (path: sf::InMapAliasBuffer) => (virtual_amiibo: amiibo::VirtualAmiiboData));
}

pub struct EmulationService {
    session: sf::Session
}

impl sf::IObject for EmulationService {
    fn get_session(&mut self) -> &mut sf::Session {
        &mut self.session
    }

    fn get_command_table(&self) -> sf::CommandMetadataTable {
        vec! [
            ipc_cmif_interface_make_command_meta!(get_version: 0),
            ipc_cmif_interface_make_command_meta!(get_virtual_amiibo_directory: 1),
            ipc_cmif_interface_make_command_meta!(get_emulation_status: 2),
            ipc_cmif_interface_make_command_meta!(set_emulation_status: 3),
            ipc_cmif_interface_make_command_meta!(get_active_virtual_amiibo: 4),
            ipc_cmif_interface_make_command_meta!(set_active_virtual_amiibo: 5),
            ipc_cmif_interface_make_command_meta!(reset_active_virtual_amiibo: 6),
            ipc_cmif_interface_make_command_meta!(get_active_virtual_amiibo_status: 7),
            ipc_cmif_interface_make_command_meta!(set_active_virtual_amiibo_status: 8),
            ipc_cmif_interface_make_command_meta!(is_application_id_intercepted: 9),
            ipc_cmif_interface_make_command_meta!(try_parse_virtual_amiibo: 10)
        ]
    }
}

impl server::IServerObject for EmulationService {
    fn new() -> Self {
        Self { session: sf::Session::new() }
    }
}

impl IEmulationService for EmulationService {
    fn get_version(&mut self) -> Result<emu::Version> {
        Ok(emu::CURRENT_VERSION)
    }

    fn get_virtual_amiibo_directory(&mut self, mut out_path: sf::OutMapAliasBuffer) -> Result<()> {
        out_path.set_string(String::from(fsext::VIRTUAL_AMIIBO_DIR));
        Ok(())
    }

    fn get_emulation_status(&mut self) -> Result<emu::EmulationStatus> {
        let status = emu::get_emulation_status();
        Ok(status)
    }

    fn set_emulation_status(&mut self, status: emu::EmulationStatus) -> Result<()> {
        emu::set_emulation_status(status);
        Ok(())
    }

    fn get_active_virtual_amiibo(&mut self, mut out_path: sf::OutMapAliasBuffer) -> Result<amiibo::VirtualAmiiboData> {
        let amiibo = emu::get_active_virtual_amiibo();
        result_return_unless!(amiibo.is_valid(), resultsext::emu::ResultInvalidVirtualAmiibo);

        let data = amiibo.produce_data()?;
        out_path.set_string(amiibo.path.clone());
        Ok(data)
    }

    fn set_active_virtual_amiibo(&mut self, path: sf::InMapAliasBuffer) -> Result<()> {
        let path_str = path.get_string();
        let amiibo = amiibo::try_load_virtual_amiibo(path_str)?;
        result_return_unless!(amiibo.is_valid(), resultsext::emu::ResultInvalidVirtualAmiibo);

        emu::set_active_virtual_amiibo(amiibo);
        Ok(())
    }

    fn reset_active_virtual_amiibo(&mut self) -> Result<()> {
        emu::set_active_virtual_amiibo(amiibo::VirtualAmiibo::empty());
        Ok(())
    }

    fn get_active_virtual_amiibo_status(&mut self) -> Result<emu::VirtualAmiiboStatus> {
        let status = emu::get_active_virtual_amiibo_status();
        Ok(status)
    }

    fn set_active_virtual_amiibo_status(&mut self, status: emu::VirtualAmiiboStatus) -> Result<()> {
        emu::set_active_virtual_amiibo_status(status);
        Ok(())
    }

    fn is_application_id_intercepted(&mut self, application_id: u64) -> Result<bool> {
        Ok(emu::is_application_id_intercepted(application_id))
    }

    fn try_parse_virtual_amiibo(&mut self, path: sf::InMapAliasBuffer) -> Result<amiibo::VirtualAmiiboData> {
        let path_str = path.get_string();
        let amiibo = amiibo::try_load_virtual_amiibo(path_str)?;
        result_return_unless!(amiibo.is_valid(), resultsext::emu::ResultInvalidVirtualAmiibo);

        let data = amiibo.produce_data()?;
        Ok(data)
    }
}

impl server::IService for EmulationService {
    fn get_name() -> &'static str {
        nul!("emuiibo")
    }

    fn get_max_sesssions() -> i32 {
        40
    }
}