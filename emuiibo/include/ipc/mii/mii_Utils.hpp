
#pragma once
#include <ipc/mii/mii_Service.hpp>

namespace ipc::mii {

    void DumpSystemMiis();

    static inline constexpr const char *NewMiiName = "emuiibo";
    
    inline CharInfo GenerateRandomMii() {
        CharInfo charinfo = {};
        BuildRandom(&charinfo, Age::All, Gender::All, Race::All);
        // Use a copy to avoid warnings, since the charinfo struct is packed
        u16 mii_name_copy[10+1] = {0};
        utf8_to_utf16(mii_name_copy, (const u8*)NewMiiName, strlen(NewMiiName));
        memcpy(charinfo.mii_name, mii_name_copy, sizeof(mii_name_copy));
        return charinfo;
    }

}