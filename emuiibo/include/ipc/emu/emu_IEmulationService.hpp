 
#pragma once
#include <ipc/ipc_Utils.hpp>
#include <sys/sys_Emulation.hpp>

namespace ipc::emu {

    constexpr ams::sm::ServiceName ServiceName = ams::sm::ServiceName::Encode("emuiibo");

    class IEmulationService final : public ams::sf::IServiceObject {

        private:
            enum class CommandId {
                GetVersion = 0,
                GetVirtualAmiiboDirectory = 1,
                GetEmulationStatus = 2,
                SetEmulationStatus = 3,
                GetActiveVirtualAmiibo = 4,
                SetActiveVirtualAmiibo = 5,
                ResetActiveVirtualAmiibo = 6,
                GetActiveVirtualAmiiboStatus = 7,
                SetActiveVirtualAmiiboStatus = 8,
                IsApplicationIdIntercepted = 9,
                IsCurrentApplicationIdIntercepted = 10,
                TryParseVirtualAmiibo = 11,
            };

        private:
            void GetVersion(ams::sf::Out<Version> out_version) {
                out_version.SetValue(CurrentVersion);
            }

            void GetVirtualAmiiboDirectory(const ams::sf::OutBuffer &out_path_buf) {
                CopyStringToOutBuffer(consts::AmiiboDir, out_path_buf);
            }

            void GetEmulationStatus(ams::sf::Out<sys::EmulationStatus> out_status) {
                auto status = sys::GetEmulationStatus();
                EMU_LOG_FMT("Emulation status: " << static_cast<u32>(status))
                out_status.SetValue(status);
            }

            void SetEmulationStatus(sys::EmulationStatus status) {
                EMU_LOG_FMT("Emulation status: " << static_cast<u32>(status))
                sys::SetEmulationStatus(status);
            }

            ams::Result GetActiveVirtualAmiibo(ams::sf::Out<amiibo::VirtualAmiiboData> out_data, const ams::sf::OutBuffer &out_path_buf) {
                auto &amiibo = sys::GetActiveVirtualAmiibo();
                R_UNLESS(amiibo.IsValid(), result::emu::ResultNoActiveVirtualAmiibo);

                out_data.SetValue(amiibo.ProduceData());
                CopyStringToOutBuffer(amiibo.GetPath(), out_path_buf);
                return ams::ResultSuccess();
            }

            ams::Result SetActiveVirtualAmiibo(const ams::sf::InBuffer &path_buf) {
                auto path = CopyStringFromInBuffer(path_buf);
                amiibo::VirtualAmiibo amiibo;
                R_UNLESS(amiibo::VirtualAmiibo::GetValidVirtualAmiibo(path, amiibo), result::emu::ResultInvalidVirtualAmiibo);

                sys::SetActiveVirtualAmiibo(amiibo);
                return ams::ResultSuccess();
            }

            void ResetActiveVirtualAmiibo() {
                amiibo::VirtualAmiibo empty_amiibo;
                sys::SetActiveVirtualAmiibo(empty_amiibo);
            }

            void GetActiveVirtualAmiiboStatus(ams::sf::Out<sys::VirtualAmiiboStatus> out_status) {
                auto status = sys::GetActiveVirtualAmiiboStatus();
                out_status.SetValue(status);
            }

            void SetActiveVirtualAmiiboStatus(sys::VirtualAmiiboStatus status) {
                sys::SetActiveVirtualAmiiboStatus(status);
            }

            void IsApplicationIdIntercepted(ams::sf::Out<bool> out_intercepted, u64 app_id) {
                out_intercepted.SetValue(sys::IsApplicationIdIntercepted(app_id));
            }

            void IsCurrentApplicationIdIntercepted(ams::sf::Out<bool> out_intercepted) {
                out_intercepted.SetValue(sys::IsCurrentApplicationIdIntercepted());
            }

            ams::Result TryParseVirtualAmiibo(const ams::sf::InBuffer &path_buf, ams::sf::Out<amiibo::VirtualAmiiboData> out_data) {
                auto path = CopyStringFromInBuffer(path_buf);
                amiibo::VirtualAmiibo amiibo;
                R_UNLESS(amiibo::VirtualAmiibo::GetValidVirtualAmiibo(path, amiibo), result::emu::ResultInvalidVirtualAmiibo);

                out_data.SetValue(amiibo.ProduceData());
                return ams::ResultSuccess();
            }

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetVersion),
                MAKE_SERVICE_COMMAND_META(GetVirtualAmiiboDirectory),
                MAKE_SERVICE_COMMAND_META(GetEmulationStatus),
                MAKE_SERVICE_COMMAND_META(SetEmulationStatus),
                MAKE_SERVICE_COMMAND_META(GetActiveVirtualAmiibo),
                MAKE_SERVICE_COMMAND_META(SetActiveVirtualAmiibo),
                MAKE_SERVICE_COMMAND_META(ResetActiveVirtualAmiibo),
                MAKE_SERVICE_COMMAND_META(GetActiveVirtualAmiiboStatus),
                MAKE_SERVICE_COMMAND_META(SetActiveVirtualAmiiboStatus),
                MAKE_SERVICE_COMMAND_META(IsApplicationIdIntercepted),
                MAKE_SERVICE_COMMAND_META(IsCurrentApplicationIdIntercepted),
                MAKE_SERVICE_COMMAND_META(TryParseVirtualAmiibo),
            };
    };

}