#include <ipc/mii/mii_Service.hpp>
#include <cstring>

namespace ipc::mii {

    static Service g_mii_srv;
    static Service g_mii_database_srv;

    Result Initialize() {
        if(serviceIsActive(&g_mii_srv)) {
            return Success;
        }
        auto rc = smGetService(&g_mii_srv, "mii:u");
        if(R_SUCCEEDED(rc)) {
            u32 in = static_cast<u32>(SpecialMiiKeyCode::Normal);
            // GetDatabaseService
            rc = serviceDispatchIn(&g_mii_srv, 0, in,
                .out_num_objects = 1,
                .out_objects = &g_mii_database_srv,
            );
        }
        return rc;
    }

    void Finalize() {
        if(serviceIsActive(&g_mii_database_srv)) {
            serviceClose(&g_mii_database_srv);
        }
        if(serviceIsActive(&g_mii_srv)) {
            serviceClose(&g_mii_srv);
        }
    }

    Result BuildRandom(CharInfo *out, Age age, Gender gender, Race race) {
        const struct {
            Age age;
            Gender gender;
            Race race;
        } in = { age, gender, race };
        return serviceDispatchInOut(&g_mii_database_srv, 6, in, *out);
    }

    Result GetCount(u32 *out_count) {
        auto in = static_cast<u32>(SourceFlag::Database);
        return serviceDispatchInOut(&g_mii_database_srv, 2, in, *out_count);
    }

    static inline Result GetCharInfos(CharInfo *buf, size_t sz, u32 *out_size) {
        auto in = static_cast<u32>(SourceFlag::Database);
        return serviceDispatchInOut(&g_mii_database_srv, 4, in, *out_size, 
            .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
            .buffers = { { buf, sz } },
        );
    };

    Result GetCharInfo(u32 idx, CharInfo *out_info) {
        // Get charinfo list and return the one we want
        u32 mii_count = 0;
        auto rc = GetCount(&mii_count);
        if(R_SUCCEEDED(rc)) {
            if(mii_count < 1) {
                return result::emu::ResultUnableToReadMii;
            }

            auto buf = new CharInfo[mii_count]();

            u32 out_size = 0;
            auto rc = GetCharInfos(buf, mii_count * sizeof(CharInfo), &out_size);

            auto size = std::min(mii_count, out_size);
            if(R_SUCCEEDED(rc)) {
                if(idx < size) {
                    memcpy(out_info, &buf[idx], sizeof(CharInfo));
                }
                else {
                    return result::emu::ResultUnableToReadMii;
                }
            }

            delete[] buf;
        }

        return rc;
    }
}