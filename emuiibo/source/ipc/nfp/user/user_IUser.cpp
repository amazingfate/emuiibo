#include <ipc/nfp/user/user_IUser.hpp>

namespace ipc::nfp::user {

    ams::Result IUser::OpenApplicationArea(DeviceHandle handle, amiibo::AreaId id, ams::sf::Out<u32> out_npad_id) {
        auto &amiibo = sys::GetActiveVirtualAmiibo();
        EMU_LOG_FMT("Open area - area ID: 0x" << std::hex << id << std::dec << ", is amiibo valid? " << std::boolalpha << amiibo.IsValid())
        R_UNLESS(this->IsStateAny<NfpState>(NfpState_Initialized), result::nfp::ResultDeviceNotFound);
        R_UNLESS(this->IsStateAny<NfpDeviceState>(NfpDeviceState_TagMounted), result::nfp::ResultDeviceNotFound);
        R_UNLESS(amiibo.IsValid(), result::nfp::ResultDeviceNotFound);

        out_npad_id.SetValue(handle.npad_id);

        auto &area_manager = amiibo.GetAreaManager();
        EMU_LOG_FMT("Open area - exists area? " << std::boolalpha << area_manager.Exists(id))
        R_UNLESS(area_manager.Exists(id), result::nfp::ResultAreaNeedsToBeCreated);

        // This area is opened now
        this->current_opened_area_id = id;
        this->area_opened = true;
        return ams::ResultSuccess();
    }

    ams::Result IUser::GetApplicationArea(const ams::sf::OutBuffer &data, ams::sf::Out<u32> data_size, DeviceHandle handle) {
        EMU_LOG_FMT("Get area - current area ID: " << std::hex << this->current_opened_area_id)
        R_UNLESS(this->area_opened, result::nfp::ResultDeviceNotFound);
        
        auto &amiibo = sys::GetActiveVirtualAmiibo();
        EMU_LOG_FMT("Get area - is amiibo valid? " << std::boolalpha << amiibo.IsValid())
        R_UNLESS(this->IsStateAny<NfpState>(NfpState_Initialized), result::nfp::ResultDeviceNotFound);
        R_UNLESS(this->IsStateAny<NfpDeviceState>(NfpDeviceState_TagMounted), result::nfp::ResultDeviceNotFound);
        R_UNLESS(amiibo.IsValid(), result::nfp::ResultDeviceNotFound);

        auto &area_manager = amiibo.GetAreaManager();
        EMU_LOG_FMT("Get area - exists area? " << std::boolalpha << area_manager.Exists(this->current_opened_area_id))
        R_UNLESS(area_manager.Exists(this->current_opened_area_id), result::nfp::ResultAreaNeedsToBeCreated);

        auto size = area_manager.GetSize(this->current_opened_area_id);
        R_UNLESS(size > 0, result::nfp::ResultAreaNeedsToBeCreated);

        area_manager.Read(this->current_opened_area_id, data.GetPointer(), size);
        data_size.SetValue(static_cast<u32>(size));
        return ams::ResultSuccess();
    }

    ams::Result IUser::SetApplicationArea(const ams::sf::InBuffer &data, DeviceHandle handle) {
        EMU_LOG_FMT("Set area - current area ID: " << std::hex << this->current_opened_area_id)
        R_UNLESS(this->area_opened, result::nfp::ResultDeviceNotFound);
        
        auto &amiibo = sys::GetActiveVirtualAmiibo();
        EMU_LOG_FMT("Set area - is amiibo valid? " << std::boolalpha << amiibo.IsValid())
        R_UNLESS(this->IsStateAny<NfpState>(NfpState_Initialized), result::nfp::ResultDeviceNotFound);
        R_UNLESS(this->IsStateAny<NfpDeviceState>(NfpDeviceState_TagMounted), result::nfp::ResultDeviceNotFound);
        R_UNLESS(amiibo.IsValid(), result::nfp::ResultDeviceNotFound);

        auto &area_manager = amiibo.GetAreaManager();
        EMU_LOG_FMT("Set area - exists area? " << std::boolalpha << area_manager.Exists(this->current_opened_area_id))
        R_UNLESS(area_manager.Exists(this->current_opened_area_id), result::nfp::ResultAreaNeedsToBeCreated);

        auto size = area_manager.GetSize(this->current_opened_area_id);
        R_UNLESS(size > 0, result::nfp::ResultAreaNeedsToBeCreated);

        area_manager.Write(this->current_opened_area_id, data.GetPointer(), data.GetSize());
        // Notify that the amiibo was written :P
        amiibo.NotifyWritten();
        return ams::ResultSuccess();
    }

    ams::Result IUser::CreateApplicationArea(const ams::sf::InBuffer &data, DeviceHandle handle, amiibo::AreaId id) {
        EMU_LOG_FMT("Create area - area ID: " << std::hex << id)
        
        auto &amiibo = sys::GetActiveVirtualAmiibo();
        EMU_LOG_FMT("Create area - is amiibo valid? " << std::boolalpha << amiibo.IsValid())
        R_UNLESS(this->IsStateAny<NfpState>(NfpState_Initialized), result::nfp::ResultDeviceNotFound);
        R_UNLESS(this->IsStateAny<NfpDeviceState>(NfpDeviceState_TagMounted), result::nfp::ResultDeviceNotFound);
        R_UNLESS(amiibo.IsValid(), result::nfp::ResultDeviceNotFound);

        auto &area_manager = amiibo.GetAreaManager();
        // If it already exists, this should not succeed
        R_UNLESS(!area_manager.Exists(id), result::nfp::ResultAreaAlreadyCreated);
        area_manager.Create(id, data.GetPointer(), data.GetSize());
        return ams::ResultSuccess();
    }

    ams::Result IUser::GetApplicationAreaSize(DeviceHandle handle, ams::sf::Out<u32> size) {
        EMU_LOG_FMT("Get area - current area ID: " << std::hex << this->current_opened_area_id)
        R_UNLESS(this->area_opened, result::nfp::ResultDeviceNotFound);
        
        auto &amiibo = sys::GetActiveVirtualAmiibo();
        EMU_LOG_FMT("Get area - is amiibo valid? " << std::boolalpha << amiibo.IsValid())
        R_UNLESS(amiibo.IsValid(), result::nfp::ResultDeviceNotFound);

        auto &area_manager = amiibo.GetAreaManager();
        EMU_LOG_FMT("Get area - exists area? " << std::boolalpha << area_manager.Exists(this->current_opened_area_id))
        R_UNLESS(area_manager.Exists(this->current_opened_area_id), result::nfp::ResultAreaNeedsToBeCreated);

        auto sz = area_manager.GetSize(this->current_opened_area_id);
        size.SetValue(static_cast<u32>(sz));
        return ams::ResultSuccess();
    }

    ams::Result IUser::RecreateApplicationArea(const ams::sf::InBuffer &data, DeviceHandle handle, amiibo::AreaId id) {
        EMU_LOG_FMT("Recreate area - current area ID: " << std::hex << id)
        R_UNLESS(this->area_opened, result::nfp::ResultDeviceNotFound);
        
        auto &amiibo = sys::GetActiveVirtualAmiibo();
        EMU_LOG_FMT("Recreate area - is amiibo valid? " << std::boolalpha << amiibo.IsValid())
        R_UNLESS(this->IsStateAny<NfpState>(NfpState_Initialized), result::nfp::ResultDeviceNotFound);
        R_UNLESS(this->IsStateAny<NfpDeviceState>(NfpDeviceState_TagMounted), result::nfp::ResultDeviceNotFound);
        R_UNLESS(amiibo.IsValid(), result::nfp::ResultDeviceNotFound);

        auto &area_manager = amiibo.GetAreaManager();
        area_manager.Recreate(id, data.GetPointer(), data.GetSize());
        return ams::ResultSuccess();
    }

}