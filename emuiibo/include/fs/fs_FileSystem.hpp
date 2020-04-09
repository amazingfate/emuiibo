
#pragma once
#include <emu_Types.hpp>
#include <iomanip>
#include <sys/stat.h>
#include <dirent.h>

namespace fs {

    inline void CreateDirectory(const std::string &path) {
        mkdir(path.c_str(), 777);
    }

    template<mode_t Mode>
    inline bool StatImpl(const std::string &path) {
        struct stat st;
        if(stat(path.c_str(), &st) == 0) {
            if(st.st_mode & Mode) {
                return true;
            }
        }
        return false;
    }

    inline bool IsFile(const std::string &path) {
        return StatImpl<S_IFREG>(path);
    }

    inline bool IsDirectory(const std::string &path) {
        return StatImpl<S_IFDIR>(path);
    }

    inline bool MatchesExtension(const std::string &path, const std::string &ext) {
        return path.substr(path.find_last_of(".") + 1) == ext;
    }

    inline std::string GetBaseName(const std::string &path) {
        return path.substr(path.find_last_of("/") + 1);
    }

    inline std::string RemoveExtension(const std::string &path) {
        return path.substr(0, path.find_last_of("."));
    }

    inline void DeleteFile(const std::string &path) {
        remove(path.c_str());
    }

    inline void DeleteDirectory(const std::string &path) {
        fsdevDeleteDirectoryRecursively(path.c_str());
    }

    inline void RecreateDirectory(const std::string &path) {
        DeleteDirectory(path);
        CreateDirectory(path);
    }

    inline void CreateEmptyFile(const std::string &path) {
        std::ofstream ofs(path);
    }

    inline size_t GetFileSize(const std::string &path) {
        struct stat st;
        if(stat(path.c_str(), &st) == 0) {
            return st.st_size;
        }
        return 0;
    }

    inline void ConcatImpl(std::string &base, const std::string &p) {
        if(base.back() != '/') {
            if(p.front() != '/') {
                base += '/';
            }
        }
        base += p;
    }

    template<typename ...Ps>
    inline std::string Concat(const std::string &base, Ps &&...paths) {
        // Generate a copy
        auto ret = base;
        (ConcatImpl(ret, paths), ...);
        return ret;
    }

    inline JSON LoadJSONFile(const std::string &path) {
        std::ifstream ifs(path);
        if(ifs.good()) {
            return JSON::parse(ifs);
        }
        return JSON::object();
    }

    inline void SaveJSONFile(const std::string &path, JSON &json) {
        std::ofstream ofs(path);
        if(ofs.good()) {
            ofs << std::setw(4) << json;
        }
    }

    template<typename T>
    inline void Save(const std::string &path, T t) {
        auto f = fopen(path.c_str(), "wb");
        if(f) {
            fwrite(&t, 1, sizeof(T), f);
            fclose(f);
        }
    }

    template<typename T>
    inline T Read(const std::string &path) {
        T t = T();
        const size_t file_sz = GetFileSize(path);
        if(file_sz >= sizeof(T)) {
            auto f = fopen(path.c_str(), "rb");
            if(f) {
                fread(&t, 1, sizeof(T), f);
                fclose(f);
            }
        }
        return t;
    }

    inline void CopyFile(const std::string &in_path, const std::string &out_path) {
        auto fsize = GetFileSize(in_path);
        auto inf = fopen(in_path.c_str(), "rb");
        if(inf) {
            auto outf = fopen(out_path.c_str(), "wb");
            if(outf) {
                if(fsize > 0) {
                    auto buf = new u8[fsize]();
                    fread(buf, 1, fsize, inf);
                    fwrite(buf, 1, fsize, outf);
                    delete[] buf;
                }
                fclose(outf);
            }
            fclose(inf);
        }
    }

    inline void EnsureEmuiiboDirectories() {
        CreateDirectory(consts::EmuDir);
        CreateDirectory(consts::AmiiboDir);
        CreateDirectory(consts::DumpedMiisDir);
        CreateDirectory(consts::TempConversionAreasDir);
    }

    #define FS_FOR(path, entry_v, path_v, ...) { \
        auto dir = opendir(path.c_str()); \
        if(dir) { \
            while(true) { \
                auto dt = readdir(dir); \
                if(dt == nullptr) { \
                    break; \
                } \
                std::string entry_v = dt->d_name; \
                auto path_v = fs::Concat(path, entry_v); \
                __VA_ARGS__ \
            } \
        } \
    }

}
