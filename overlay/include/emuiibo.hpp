
#pragma once
#include <vector>
#include <string>
#include <switch.h>
#include <cstring>

namespace emu {

    struct VirtualAmiiboId {
        u8 id[0x10];

        inline constexpr bool Equals(VirtualAmiiboId id) {
            return memcmp(this->id, id.id, 0x10) == 0;
        }
    };

    struct VirtualAmiiboUuidInfo {
        bool random_uuid;
        u8 uuid[10];
    };

    struct VirtualAmiiboDate {
        u16 year;
        u8 month;
        u8 day;
    };

    struct VirtualAmiiboData {
        VirtualAmiiboUuidInfo uuid;
        char name[40 + 1];
        char path[FS_MAX_PATH];
        VirtualAmiiboDate first_write_date;
        VirtualAmiiboDate last_write_date;
        NfpMiiCharInfo mii_charinfo;

        inline bool IsValid() {
            return strlen(this->name) && strlen(this->path);
        }

    };

    enum class EmulationStatus : u32 {
        On,
        Off,
    };

    enum class VirtualAmiiboStatus : u32 {
        Invalid,
        Connected,
        Disconnected
    };

    struct Version {
        u8 major;
        u8 minor;
        u8 micro;
        bool dev_build;
    };

    bool IsAvailable();

    Result Initialize();
    void Exit();

    Version GetVersion();

    EmulationStatus GetEmulationStatus();
    void SetEmulationStatus(EmulationStatus status);

    Result GetActiveVirtualAmiibo(VirtualAmiiboId *out_amiibo_id, VirtualAmiiboData *out_amiibo_data);
    Result SetActiveVirtualAmiibo(VirtualAmiiboId *amiibo_id);
    void ResetActiveVirtualAmiibo();

    VirtualAmiiboStatus GetActiveVirtualAmiiboStatus();
    void SetActiveVirtualAmiiboStatus(VirtualAmiiboStatus status);

    Result ReadNextAvailableVirtualAmiibo(VirtualAmiiboId *out_amiibo_id, VirtualAmiiboData *out_amiibo_data);
    void ResetAvailableVirtualAmiiboIterator();

    void IsApplicationIdIntercepted(u64 app_id, bool *out_intercepted);
    void IsCurrentApplicationIdIntercepted(bool *out_intercepted);

}