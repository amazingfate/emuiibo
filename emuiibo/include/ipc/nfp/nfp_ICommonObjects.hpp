 
#pragma once
#include <ipc/nfp/nfp_Types.hpp>
#include <emu_Results.hpp>
#include <sys/sys_Emulation.hpp>

namespace ipc::nfp {

    // All nfp interfaces share this commands, so have several macros to simplify the code

    #define NFP_COMMON_IFACE_COMMAND_IDS \
        Initialize = 0, \
        Finalize = 1, \
        ListDevices = 2, \
        StartDetection = 3, \
        StopDetection = 4, \
        Mount = 5, \
        Unmount = 6, \
        Flush = 10, \
        Restore = 11, \
        GetTagInfo = 13, \
        GetRegisterInfo = 14, \
        GetCommonInfo = 15, \
        GetModelInfo = 16, \
        AttachActivateEvent = 17, \
        AttachDeactivateEvent = 18, \
        GetState = 19, \
        GetDeviceState = 20, \
        GetNpadId = 21, \
        AttachAvailabilityChangeEvent = 23

    #define NFP_COMMON_IFACE_COMMAND_METAS \
        MAKE_SERVICE_COMMAND_META(Initialize), \
        MAKE_SERVICE_COMMAND_META(Finalize), \
        MAKE_SERVICE_COMMAND_META(ListDevices), \
        MAKE_SERVICE_COMMAND_META(StartDetection), \
        MAKE_SERVICE_COMMAND_META(StopDetection), \
        MAKE_SERVICE_COMMAND_META(Mount), \
        MAKE_SERVICE_COMMAND_META(Unmount), \
        MAKE_SERVICE_COMMAND_META(Flush), \
        MAKE_SERVICE_COMMAND_META(Restore), \
        MAKE_SERVICE_COMMAND_META(GetTagInfo), \
        MAKE_SERVICE_COMMAND_META(GetRegisterInfo), \
        MAKE_SERVICE_COMMAND_META(GetCommonInfo), \
        MAKE_SERVICE_COMMAND_META(GetModelInfo), \
        MAKE_SERVICE_COMMAND_META(AttachActivateEvent), \
        MAKE_SERVICE_COMMAND_META(AttachDeactivateEvent), \
        MAKE_SERVICE_COMMAND_META(GetState), \
        MAKE_SERVICE_COMMAND_META(GetDeviceState), \
        MAKE_SERVICE_COMMAND_META(GetNpadId), \
        MAKE_SERVICE_COMMAND_META(AttachAvailabilityChangeEvent)

    #define NFP_USE_CTOR_OF(cls) \
        public: \
        using cls::cls;

    class ICommonInterface : public ams::sf::IServiceObject {

        public:
            static constexpr u32 HandheldNpadId = 0x20;
            static constexpr u32 Player1NpadId = 0;

        protected:
            NfpState state;
            NfpDeviceState device_state;
            ams::os::SystemEventType event_activate;
            ams::os::SystemEventType event_deactivate;
            ams::os::SystemEventType event_availability_change;
            Service *forward_service;
            u64 client_app_id;

            Lock amiibo_update_lock;
            sys::VirtualAmiiboStatus last_notified_status;
            Thread amiibo_update_thread;
            bool should_exit_thread;

        protected:
            NfpState GetStateValue();
            void SetStateValue(NfpState val);
            NfpDeviceState GetDeviceStateValue();
            void SetDeviceStateValue(NfpDeviceState val);

            inline void NotifyShouldExitThread() {
                EMU_LOCK_SCOPE_WITH(this->amiibo_update_lock);
                this->should_exit_thread = true;
            }

            inline void NotifyThreadExitAndWait() {
                this->NotifyShouldExitThread();
                EMU_R_ASSERT(threadWaitForExit(&this->amiibo_update_thread));
            }

            template<typename T>
            inline constexpr void IsInStateImpl(bool &out, T base, T state) {
                if(base == state) {
                    out = true;
                }
            }

            template<typename T, typename ...Ss>
            inline constexpr bool IsStateAny(Ss &&...states) {
                static_assert(std::is_same_v<T, NfpState> || std::is_same_v<T, NfpDeviceState>, "Invalid type");
                bool ret = false;
                if constexpr(std::is_same_v<T, NfpState>) {
                    auto state = this->GetStateValue();
                    (IsInStateImpl(ret, state, states), ...);
                    return ret;
                }
                else if constexpr(std::is_same_v<T, NfpDeviceState>) {
                    auto state = this->GetDeviceStateValue();
                    (IsInStateImpl(ret, state, states), ...);
                    return ret;
                }
                return false;
            }

        public:
            ICommonInterface(Service *fwd, u64 app_id);
            ~ICommonInterface();

            void HandleVirtualAmiiboStatus(sys::VirtualAmiiboStatus status);

            inline bool ShouldExitThread() {
                EMU_LOCK_SCOPE_WITH(this->amiibo_update_lock);
                return this->should_exit_thread;
            }

        protected:
            void Initialize(const ams::sf::ClientAppletResourceUserId &client_aruid, const ams::sf::ClientProcessId &client_pid, const ams::sf::InBuffer &mcu_data);
            void Finalize();
            ams::Result ListDevices(const ams::sf::OutPointerArray<DeviceHandle> &out_devices, ams::sf::Out<s32> out_count);
            ams::Result StartDetection(DeviceHandle handle);
            ams::Result StopDetection(DeviceHandle handle);
            ams::Result Mount(DeviceHandle handle, u32 type, u32 target);
            ams::Result Unmount(DeviceHandle handle);
            ams::Result Flush(DeviceHandle handle);
            ams::Result Restore(DeviceHandle handle);
            ams::Result GetTagInfo(ams::sf::Out<TagInfo> out_info, DeviceHandle handle);
            ams::Result GetRegisterInfo(ams::sf::Out<RegisterInfo> out_info, DeviceHandle handle);
            ams::Result GetCommonInfo(ams::sf::Out<CommonInfo> out_info, DeviceHandle handle);
            ams::Result GetModelInfo(ams::sf::Out<ModelInfo> out_info, DeviceHandle handle);
            ams::Result AttachActivateEvent(DeviceHandle handle, ams::sf::Out<ams::sf::CopyHandle> event);
            ams::Result AttachDeactivateEvent(DeviceHandle handle, ams::sf::Out<ams::sf::CopyHandle> event);
            void GetState(ams::sf::Out<u32> state);
            void GetDeviceState(DeviceHandle handle, ams::sf::Out<u32> state);
            ams::Result GetNpadId(DeviceHandle handle, ams::sf::Out<u32> npad_id);
            ams::Result AttachAvailabilityChangeEvent(ams::sf::Out<ams::sf::CopyHandle> event);

    };

    #define NFP_COMMON_MANAGER_BASE \
        protected: \
            enum class CommandId { \
                CreateInterface = 0, \
            }; \
        public: \
            DEFINE_SERVICE_DISPATCH_TABLE { \
                MAKE_SERVICE_COMMAND_META(CreateInterface), \
            };

    #define NFP_COMMON_MANAGER_CREATE_CMD(type) \
        protected: \
            ams::Result CreateInterface(ams::sf::Out<std::shared_ptr<type>> out) { \
                Service outsrv; \
                R_TRY(ICommonManager::CreateForwardInterface(this->forward_service.get(), &outsrv)); \
                const ams::sf::cmif::DomainObjectId object_id { serviceGetObjectId(&outsrv) }; \
                auto intf = std::make_shared<type>(new Service(outsrv), this->client_app_id); \
                out.SetValue(std::move(intf), object_id); \
                return ams::ResultSuccess(); \
            }

    class ICommonManager : public ams::sf::IMitmServiceObject {

        protected:
            u64 client_app_id;

        public:
            ICommonManager(std::shared_ptr<::Service> &&s, const ams::sm::MitmProcessInfo &c) : IMitmServiceObject(std::forward<std::shared_ptr<::Service>>(s), c), client_app_id(c.program_id.value) {
                EMU_LOG_FMT("Accessed manager with application ID 0x" << std::hex << std::setw(16) << std::setfill('0') << std::uppercase << this->client_app_id)
            }

            static bool ShouldMitm(const ams::sm::MitmProcessInfo &client_info) {
                return sys::GetEmulationStatus() == sys::EmulationStatus::On;
            }

            static ams::Result CreateForwardInterface(Service *manager, Service *out);

    };
}