 
#pragma once
#include <ipc/ipc_Utils.hpp>
#include <sys/sys_Emulation.hpp>
#include <sys/sys_Locator.hpp>

namespace ipc::emu {

    constexpr ams::sm::ServiceName ServiceName = ams::sm::ServiceName::Encode("nfp:emu");

    class IEmulationService final : public ams::sf::IServiceObject {

        private:
            enum class CommandId {
                GetVersion = 0,
                GetEmulationStatus = 1,
                SetEmulationStatus = 2,
                GetActiveVirtualAmiibo = 3,
                SetActiveVirtualAmiibo = 4,
                ResetActiveVirtualAmiibo = 5,
                GetActiveVirtualAmiiboStatus = 6,
                SetActiveVirtualAmiiboStatus = 7,
                ReadNextAvailableVirtualAmiibo = 8,
                ResetAvailableVirtualAmiiboIterator = 9,
                IsApplicationIdIntercepted = 10,
                IsCurrentApplicationIdIntercepted = 11,
            };

        private:
            sys::VirtualAmiiboIterator amiibo_iterator;

        private:
            void GetVersion(ams::sf::Out<Version> out_version) {
                out_version.SetValue(CurrentVersion);
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

            ams::Result GetActiveVirtualAmiibo(ams::sf::Out<amiibo::VirtualAmiiboId> out_id, ams::sf::Out<amiibo::VirtualAmiiboData> out_data) {
                auto &amiibo = sys::GetActiveVirtualAmiibo();
                R_UNLESS(amiibo.IsValid(), result::emu::ResultNoActiveVirtualAmiibo);

                out_id.SetValue(amiibo.ProduceId());
                out_data.SetValue(amiibo.ProduceData());
                return ams::ResultSuccess();
            }

            ams::Result SetActiveVirtualAmiibo(amiibo::VirtualAmiiboId amiibo_id) {
                // Find the amiibo with a new iterator
                sys::VirtualAmiiboIterator iterator;
                iterator.Initialize(consts::AmiiboDir);
                while(true) {
                    amiibo::VirtualAmiibo amiibo;
                    auto ok = iterator.NextEntry(amiibo);
                    if(!ok) {
                        break;
                    }
                    if(amiibo_id.Equals(amiibo.ProduceId())) {
                        sys::SetActiveVirtualAmiibo(amiibo);
                        return ams::ResultSuccess();
                    }
                }
                iterator.Finalize();
                return result::emu::ResultInvalidVirtualAmiiboId;
            }

            void ResetActiveVirtualAmiibo() {
                EMU_LOG_FMT("Resetting active virtual amiibo...")
                amiibo::VirtualAmiibo empty_amiibo;
                sys::SetActiveVirtualAmiibo(empty_amiibo);
            }

            void GetActiveVirtualAmiiboStatus(ams::sf::Out<sys::VirtualAmiiboStatus> out_status) {
                auto status = sys::GetActiveVirtualAmiiboStatus();
                EMU_LOG_FMT("Virtual amiibo status: " << static_cast<u32>(status))
                out_status.SetValue(status);
            }

            void SetActiveVirtualAmiiboStatus(sys::VirtualAmiiboStatus status) {
                EMU_LOG_FMT("Virtual amiibo status: " << static_cast<u32>(status))
                sys::SetActiveVirtualAmiiboStatus(status);
            }

            ams::Result ReadNextAvailableVirtualAmiibo(ams::sf::Out<amiibo::VirtualAmiiboId> out_id, ams::sf::Out<amiibo::VirtualAmiiboData> out_data) {
                amiibo::VirtualAmiibo amiibo;
                bool ok = this->amiibo_iterator.NextEntry(amiibo);
                R_UNLESS(ok, result::emu::ResultIteratorEndReached);

                out_id.SetValue(amiibo.ProduceId());
                out_data.SetValue(amiibo.ProduceData());
                return ams::ResultSuccess();
            }

            void ResetAvailableVirtualAmiiboIterator() {
                this->amiibo_iterator.Reset();
            }

            void IsApplicationIdIntercepted(ams::sf::Out<bool> out_intercepted, u64 app_id) {
                out_intercepted.SetValue(sys::IsApplicationIdIntercepted(app_id));
            }

            void IsCurrentApplicationIdIntercepted(ams::sf::Out<bool> out_intercepted) {
                out_intercepted.SetValue(sys::IsCurrentApplicationIdIntercepted());
            }
        
        public:
            IEmulationService() {
                this->amiibo_iterator.Initialize(consts::AmiiboDir);
            }

            ~IEmulationService() {
                this->amiibo_iterator.Finalize();
            }

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetVersion),
                MAKE_SERVICE_COMMAND_META(GetEmulationStatus),
                MAKE_SERVICE_COMMAND_META(SetEmulationStatus),
                MAKE_SERVICE_COMMAND_META(GetActiveVirtualAmiibo),
                MAKE_SERVICE_COMMAND_META(SetActiveVirtualAmiibo),
                MAKE_SERVICE_COMMAND_META(ResetActiveVirtualAmiibo),
                MAKE_SERVICE_COMMAND_META(GetActiveVirtualAmiiboStatus),
                MAKE_SERVICE_COMMAND_META(SetActiveVirtualAmiiboStatus),
                MAKE_SERVICE_COMMAND_META(ReadNextAvailableVirtualAmiibo),
                MAKE_SERVICE_COMMAND_META(ResetAvailableVirtualAmiiboIterator),
                MAKE_SERVICE_COMMAND_META(IsApplicationIdIntercepted),
                MAKE_SERVICE_COMMAND_META(IsCurrentApplicationIdIntercepted),
            };
    };

}