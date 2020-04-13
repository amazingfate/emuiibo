
#pragma once
#include <stratosphere.hpp>

namespace ipc {

    inline void CopyStringToOutBuffer(const std::string &str, const ams::sf::OutBuffer &out_buf) {
        const size_t str_len = std::min(str.length(), out_buf.GetSize());
        strncpy(reinterpret_cast<char*>(out_buf.GetPointer()), str.c_str(), str_len);
    }

    inline std::string CopyStringFromInBuffer(const ams::sf::InBuffer &buf) {
        const size_t str_len = buf.GetSize();
        char str_buf[str_len + 1] = {0};
        strncpy(str_buf, reinterpret_cast<const char*>(buf.GetPointer()), str_len);
        return str_buf;
    }

}