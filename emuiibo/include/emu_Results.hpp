
#pragma once
#include <emu_Types.hpp>

namespace result {

    // Official NFC/NFP sysmodule's results

    namespace nfp {

        static constexpr u32 Module = 115;

        EMU_DEFINE_RESULT(DeviceNotFound, 64)
        EMU_DEFINE_RESULT(NeedRestart, 96)
        EMU_DEFINE_RESULT(AreaNeedsToBeCreated, 128)
        EMU_DEFINE_RESULT(AccessIdMismatch, 152)
        EMU_DEFINE_RESULT(AreaAlreadyCreated, 168)

    }
    
    // Our results

    namespace emu {

        static constexpr u32 Module = 352; // Like emuiibo's program ID (0100000000000352)

        EMU_DEFINE_RESULT(NoActiveVirtualAmiibo, 1)
        EMU_DEFINE_RESULT(InvalidVirtualAmiiboId, 2)
        EMU_DEFINE_RESULT(IteratorEndReached, 3)
        EMU_DEFINE_RESULT(UnableToReadMii, 4)

    }

    #define EMU_R_ASSERT(rc) { \
        auto res = (rc); \
        if(R_FAILED(res)) { \
            fatalThrow(static_cast<ams::Result>(res).GetValue()); \
        } \
    }

}