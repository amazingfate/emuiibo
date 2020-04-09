
#pragma once
#include <sys/sys_Emulation.hpp>
#include <vector>
#include <dirent.h>

namespace sys {

    // Used to locate amiibos and loading them one by one

    class VirtualAmiiboIterator {

        private:
            struct DirectoryHandle {
                DIR *handle;
                std::string path;

                DirectoryHandle() : handle(nullptr) {}

                inline constexpr bool IsValid() {
                    return this->handle != nullptr;
                }

                inline void Close() {
                    if(this->IsValid()) {
                        closedir(this->handle);
                        this->handle = nullptr;
                        this->path = "";
                    }
                }
            };

        private:
            DirectoryHandle dir_handle;
            std::vector<DirectoryHandle> inner_dir_handles;

            inline DirectoryHandle OpenDirectoryHandle(const std::string &path) {
                DirectoryHandle handle = {};
                handle.handle = opendir(path.c_str());
                handle.path = path;
                return handle;
            }

            inline void OpenSubDirectory(const std::string &path) {
                auto dir = OpenDirectoryHandle(path);
                this->inner_dir_handles.push_back(dir);
            }

            inline void CloseLastDirectory() {
                if(!this->inner_dir_handles.empty()) {
                    auto dir = this->inner_dir_handles.back();
                    dir.Close();
                    this->inner_dir_handles.pop_back();
                }
            }

            inline DirectoryHandle GetCurrentDirectory() {
                if(this->inner_dir_handles.empty()) {
                    return this->dir_handle;
                }
                return this->inner_dir_handles.back();
            }

            bool Next(amiibo::VirtualAmiibo &out_amiibo);

        public:
            inline void Initialize(const std::string &path) {
                if(!this->dir_handle.IsValid()) {
                    this->dir_handle = OpenDirectoryHandle(path);
                }
            }

            inline void Finalize() {
                for(auto dir: this->inner_dir_handles) {
                    dir.Close();
                }
                this->inner_dir_handles.clear();
                this->dir_handle.Close();
            }

            inline bool NextEntry(amiibo::VirtualAmiibo &out_amiibo) {
                return this->Next(out_amiibo);
            }

            inline void Reset() {
                // Reset = finalize and initialize again
                auto root_path = this->dir_handle.path;
                this->Finalize();
                this->Initialize(root_path);
            }

    };

}