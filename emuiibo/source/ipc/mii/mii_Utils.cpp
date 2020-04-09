#include <ipc/mii/mii_Utils.hpp>
#include <fs/fs_FileSystem.hpp>

namespace ipc::mii {

    void DumpSystemMiis() {
        fs::EnsureEmuiiboDirectories();
        u32 mii_count = 0;
        auto rc = GetCount(&mii_count);
        if(R_SUCCEEDED(rc)) {
            for(u32 i = 0; i < mii_count; i++) {
                CharInfo charinfo = {};
                rc = GetCharInfo(i, &charinfo);
                if(R_SUCCEEDED(rc)) {
                    const size_t mii_name_len = 10;
                    char mii_name[mii_name_len + 1] = {0};
                    // Use a copy to avoid warnings, since the charinfo struct is packed
                    u16 mii_name_16[mii_name_len + 1] = {0};
                    memcpy(mii_name_16, charinfo.mii_name, mii_name_len);
                    utf16_to_utf8((u8*)mii_name, (const u16*)mii_name_16, mii_name_len);
                    auto charinfo_dir = std::to_string(i) + " - " + mii_name;
                    auto charinfo_dir_path = fs::Concat(consts::DumpedMiisDir, charinfo_dir);
                    fs::CreateDirectory(charinfo_dir_path);
                    auto charinfo_file_path = fs::Concat(charinfo_dir_path, "mii-charinfo.bin");
                    auto f = fopen(charinfo_file_path.c_str(), "wb");
                    if(f) {
                        fwrite(&charinfo, 1, sizeof(charinfo), f);
                        fclose(f);
                    }
                }
            }
        }
    }

}