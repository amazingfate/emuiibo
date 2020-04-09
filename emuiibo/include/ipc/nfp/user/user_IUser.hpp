 
#pragma once
#include <ipc/nfp/nfp_ICommonObjects.hpp>

namespace ipc::nfp::user
{
    class IUser : public ICommonInterface {

        private:
            enum class CommandId {
                NFP_COMMON_IFACE_COMMAND_IDS,

                // Only for nfp:user!
                OpenApplicationArea = 7,
                GetApplicationArea = 8,
                SetApplicationArea = 9,
                CreateApplicationArea = 12,
                GetApplicationAreaSize = 22,
                RecreateApplicationArea = 24,
            };

        private:
            amiibo::AreaId current_opened_area_id;
            bool area_opened;

        public:
            IUser(Service *fwd, u64 app_id) : ICommonInterface(fwd, app_id), current_opened_area_id(0), area_opened(false) {}

        protected:
            ams::Result OpenApplicationArea(DeviceHandle handle, amiibo::AreaId id, ams::sf::Out<u32> out_npad_id);
            ams::Result GetApplicationArea(const ams::sf::OutBuffer &data, ams::sf::Out<u32> data_size, DeviceHandle handle);
            ams::Result SetApplicationArea(const ams::sf::InBuffer &data, DeviceHandle handle);
            ams::Result CreateApplicationArea(const ams::sf::InBuffer &data, DeviceHandle handle, amiibo::AreaId id);
            ams::Result GetApplicationAreaSize(DeviceHandle handle, ams::sf::Out<u32> size);
            ams::Result RecreateApplicationArea(const ams::sf::InBuffer &data, DeviceHandle handle, amiibo::AreaId id);

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                NFP_COMMON_IFACE_COMMAND_METAS,

                // Only for nfp:user!
                MAKE_SERVICE_COMMAND_META(OpenApplicationArea),
                MAKE_SERVICE_COMMAND_META(GetApplicationArea),
                MAKE_SERVICE_COMMAND_META(SetApplicationArea),
                MAKE_SERVICE_COMMAND_META(CreateApplicationArea),
                MAKE_SERVICE_COMMAND_META(GetApplicationAreaSize),
                MAKE_SERVICE_COMMAND_META(RecreateApplicationArea),
            };
    };
}