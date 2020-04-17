#include <sys/sys_System.hpp>
#include <fs/fs_FileSystem.hpp>
#include <amiibo/amiibo_Formats.hpp>
#include <dirent.h>

namespace sys {

    static void ScanAmiiboDirectoryImpl(const std::string &base_path) {
        FS_FOR(base_path, entry, path, {
            // Process and convert outdated virtual amiibo formats
            if(amiibo::VirtualAmiibo::IsValidVirtualAmiiboType<amiibo::VirtualBinAmiibo>(path)) {
                EMU_LOG_FMT("Converting raw bin at '" << path << "'...")
                auto ret = amiibo::VirtualAmiibo::ConvertVirtualAmiibo<amiibo::VirtualBinAmiibo>(path);
                EMU_LOG_FMT("Conversion succeeded? " << std::boolalpha << ret << "...")
            }
            else if(amiibo::VirtualAmiibo::IsValidVirtualAmiiboType<amiibo::VirtualAmiiboV3>(path)) {
                EMU_LOG_FMT("Converting V3 (0.3.x/0.4) virtual amiibo at '" << path << "'...")
                auto ret = amiibo::VirtualAmiibo::ConvertVirtualAmiibo<amiibo::VirtualAmiiboV3>(path);
                EMU_LOG_FMT("Conversion succeeded? " << std::boolalpha << ret << "...")
            }
            // Check that it isn't a valid amiibo (it would attempt to convert mii charinfo or area bins otherwise)
            else if(!amiibo::VirtualAmiibo::IsValidVirtualAmiiboType<amiibo::VirtualAmiibo>(path)) {
                // If it's a directory, scan amiibos there too
                if(fs::IsDirectory(path)) {
                    ScanAmiiboDirectoryImpl(path);
                }
            }
        });
    }

    void ScanAmiiboDirectory() {
        ScanAmiiboDirectoryImpl(consts::AmiiboDir);
    }

    void DumpConsoleMiis() {
        fs::EnsureEmuiiboDirectories();
        MiiDatabase db;
        auto rc = miiOpenDatabase(&db, MiiSpecialKeyCode_Normal);
        if(R_SUCCEEDED(rc)) {
            s32 count = 0;
            auto flag = MiiSourceFlag_Database;
            rc = miiDatabaseGetCount(&db, &count, flag);
            if(R_SUCCEEDED(rc)) {
                if(count > 0) {
                    auto buf = new MiiCharInfo[count]();
                    s32 total = 0;
                    rc = miiDatabaseGet1(&db, flag, buf, count, &total);
                    if(R_SUCCEEDED(rc)) {
                        for(s32 i = 0; i < total; i++) {
                            auto charinfo = buf[i];
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
                    delete[] buf;
                }
            }
            miiDatabaseClose(&db);
        }
    }

}