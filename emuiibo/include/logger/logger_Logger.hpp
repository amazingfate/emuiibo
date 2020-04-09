
#pragma once
#include <emu_Consts.hpp>
#include <sstream>

namespace logger {

    void Log(const std::string &fn, const std::string &msg);
    void ClearLogs();

}

#define EMU_LOG_FMT(...) { \
    std::stringstream strm; \
    strm << __VA_ARGS__; \
    logger::Log(__PRETTY_FUNCTION__, strm.str()); \
}
