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

}