#include <sys/sys_Locator.hpp>
#include <fs/fs_FileSystem.hpp>
#include <amiibo/amiibo_Formats.hpp>
#include <dirent.h>
#include <vector>

namespace sys {

    bool VirtualAmiiboIterator::Next(amiibo::VirtualAmiibo &out_amiibo) {
        auto dir = this->GetCurrentDirectory();
        auto entry = readdir(dir.handle);
        if(entry != nullptr) {
            auto path = fs::Concat(dir.path, entry->d_name);
            if(amiibo::VirtualAmiibo::GetValidVirtualAmiibo<amiibo::VirtualAmiibo>(path, out_amiibo)) {
                return true;
            }
            else if(fs::IsDirectory(path)) {
                // Open subdir and iterate through it
                this->OpenSubDirectory(path);
                // Running Next() now will scan on this new dir
                return this->Next(out_amiibo);
            }
            else {
                // Continue until we reach the end or we actually find a valid one
                return this->Next(out_amiibo);
            }
        }
        // If this is the end of a subdir, continue on the parent one
        if(!this->inner_dir_handles.empty()) {
            this->CloseLastDirectory();
            // Running Next() now will scan on the parent dir
            return Next(out_amiibo);
        }
        return false;
    }

}