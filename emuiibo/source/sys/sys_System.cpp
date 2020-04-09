#include <sys/sys_System.hpp>
#include <fs/fs_FileSystem.hpp>
#include <amiibo/amiibo_Formats.hpp>
#include <dirent.h>

namespace sys {

    static void ScanAmiiboDirectoryImpl(const std::string &base_path) {
        auto dir = opendir(base_path.c_str());
        if(dir) {
            while(true) {
                auto dt = readdir(dir);
                if(dt == nullptr) {
                    break;
                }
                auto path = fs::Concat(base_path, dt->d_name);
                // Process and convert outdated virtual amiibo formats
                if(amiibo::VirtualAmiibo::IsValidVirtualAmiibo<amiibo::VirtualBinAmiibo>(path)) {
                    EMU_LOG_FMT("Converting raw bin at '" << path << "'...")
                    // TODO: process raw bin files
                }
                else if(amiibo::VirtualAmiibo::IsValidVirtualAmiibo<amiibo::VirtualAmiiboV2>(path)) {
                    EMU_LOG_FMT("Converting V2 (0.2.x) virtual amiibo at '" << path << "'...")
                    // TODO: process V2 amiibos
                }
                else if(amiibo::VirtualAmiibo::IsValidVirtualAmiibo<amiibo::VirtualAmiiboV3>(path)) {
                    EMU_LOG_FMT("Converting V3 (0.3.x/0.4) virtual amiibo at '" << path << "'...")
                    auto ret = amiibo::VirtualAmiibo::ConvertVirtualAmiibo<amiibo::VirtualAmiiboV3>(path);
                    EMU_LOG_FMT("Conversion succeeded? " << std::boolalpha << ret << "...")
                }
                // Check that it isn't a valid amiibo (it would attempt to convert mii charinfo or area bins otherwise)
                else if(!amiibo::VirtualAmiibo::IsValidVirtualAmiibo<amiibo::VirtualAmiibo>(path)) {
                    // If it's a directory, scan amiibos there too
                    if(fs::IsDirectory(path)) {
                        ScanAmiiboDirectoryImpl(path);
                    }
                }
            }
            closedir(dir);
        }
    }

    void ScanAmiiboDirectory() {
        ScanAmiiboDirectoryImpl(consts::AmiiboDir);
    }

}