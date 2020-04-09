
#pragma once
#include <switch.h>
#include <sstream>
#include <fstream>
#include <mutex>
#include <stratosphere.hpp>
#include <json.hpp>

#include <emu_Consts.hpp>
#include <logger/logger_Logger.hpp>

using i32 = s32;

static inline constexpr Result Success = 0;

using JSON = nlohmann::json;

struct Version {
    u8 major;
    u8 minor;
    u8 micro;
    bool dev_build;
};

static_assert(sizeof(Version) == sizeof(u32), "Invalid Version struct!");

static inline constexpr Version CurrentVersion = { EMUIIBO_MAJOR, EMUIIBO_MINOR, EMUIIBO_MICRO, EMUIIBO_DEV };

#define EMU_DEFINE_RESULT(name, desc) static constexpr Result Result##name = MAKERESULT(Module, desc);

using Lock = ams::os::RecursiveMutex;

#define EMU_LOCK_SCOPE_WITH(mtx_name) std::scoped_lock lk(mtx_name);

#define EMU_DO_UNLESS(cond, ...) ({ if(!(cond)) { __VA_ARGS__ } })