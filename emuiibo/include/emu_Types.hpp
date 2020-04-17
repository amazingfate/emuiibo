
#pragma once
#include <emu_Base.hpp>

// An amiibo ID

struct CharacterId {
    u16 game_character_id;
    u8 character_variant;
} PACKED;

static_assert(sizeof(CharacterId) == 3, "Invalid CharacterId type");

// Amiibo ID format in 3DS and in amiibo bin dumps (the one used in old emuiibo formats too)

struct OldAmiiboId {
    CharacterId character_id;
    u8 figure_type;
    u16 model_number;
    u8 series;
    u8 unk_2;
};

static_assert(sizeof(OldAmiiboId) == 8, "Invalid OldAmiiboId type");

// Amiibo ID format used in the console

struct AmiiboId {
    CharacterId character_id;
    u8 series;
    u16 model_number;
    u8 figure_type;

    static inline constexpr AmiiboId FromOldAmiiboId(OldAmiiboId old_id) {
        AmiiboId id = {};
        id.character_id = old_id.character_id;
        id.series = old_id.series;
        id.model_number = old_id.model_number;
        id.figure_type = old_id.figure_type;
        return id;
    }

    inline constexpr u64 Encode() {
        union {
            u64 v;
            u8 u[8];
        } converter = {0};
        for(u32 i = 0; i < sizeof(AmiiboId); i++) {
            converter.u[i] = ((u8*)this)[i];
        }
        return converter.v;
    }

} PACKED;

static_assert(sizeof(AmiiboId) == 7, "Invalid AmiiboId type");

// IPC wrappers for NFP amiibo data structures

struct TagInfo : public ams::sf::LargeData {
    NfpTagInfo info;
};

// More detailed amiibo info structs

struct Date {
    u16 year;
    u8 month;
    u8 day;
};

static_assert(sizeof(Date) == 4, "Invalid Date type");

struct ModelInfoImpl {
    AmiiboId id;
    u8 reserved[0x39];
} PACKED;

static_assert(sizeof(ModelInfoImpl) == sizeof(NfpModelInfo), "Invalid ModelInfo type");

struct ModelInfo : public ams::sf::LargeData {
    ModelInfoImpl info;
};

struct RegisterInfoImpl {
    static inline constexpr size_t AmiiboNameLength = 40;

    MiiCharInfo mii;
    Date first_write_date;
    char name[AmiiboNameLength + 1];
    u8 font_region;
    u8 reserved[0x7a];
} PACKED;

static_assert(sizeof(RegisterInfoImpl) == sizeof(NfpRegisterInfo), "Invalid RegisterInfo type");

struct RegisterInfo : public ams::sf::LargeData {
    RegisterInfoImpl info;
};

struct CommonInfo : public ams::sf::LargeData {
    NfpCommonInfo info;
};

// TODO: RE this struct

struct AdminInfo : public ams::sf::LargeData {
    u8 raw[0x40];
};