
#pragma once
#include <stratosphere.hpp>

namespace ipc {

    inline void CopyStringToOutBuffer(const std::string &str, const ams::sf::OutBuffer &out_buf) {
        const size_t str_len = std::min(str.length(), out_buf.GetSize());
        strncpy(reinterpret_cast<char*>(out_buf.GetPointer()), str.c_str(), str_len);
    }

}