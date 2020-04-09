#include <logger/logger_Logger.hpp>
#include <cstdio>
#include <fs/fs_FileSystem.hpp>

namespace logger {

    static Lock g_logging_lock;

    // Lock logs to avoid race conditions from multiple threads

    void Log(const std::string &fn, const std::string &msg) {
        EMU_LOCK_SCOPE_WITH(g_logging_lock);
        auto f = fopen(consts::LogFilePath.c_str(), "a+");
        if(f) {
            fprintf(f, "[ emuiibo v%s | %s ] %s\n", EMUIIBO_VERSION, fn.c_str(), msg.c_str());
            fclose(f);
        }
    }

    void ClearLogs() {
        EMU_LOCK_SCOPE_WITH(g_logging_lock);
        fs::DeleteFile(consts::LogFilePath);
    }

}