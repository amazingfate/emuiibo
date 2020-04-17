#include <amiibo/amiibo_Formats.hpp>
#include <algorithm>

namespace amiibo {

    void VirtualAmiibo::Save() {
        fs::CreateDirectory(this->path);
        auto amiibo_flag = fs::Concat(this->path, "amiibo.flag");
        fs::CreateEmptyFile(amiibo_flag);
        auto json_file = fs::Concat(this->path, "amiibo.json");
        fs::DeleteFile(json_file);
        fs::SaveJSONFile(json_file, this->amiibo_data);
    }

    VirtualAmiibo::VirtualAmiibo(const std::string &amiibo_path) : IVirtualAmiiboBase(amiibo_path), area_manager(amiibo_path) {
        this->amiibo_data = fs::LoadJSONFile(fs::Concat(amiibo_path, "amiibo.json"));
        // Some checks to see if the amiibo is correct
        EMU_DO_UNLESS(!this->amiibo_data.empty(), this->valid = false;);
        EMU_DO_UNLESS(this->HasKey(this->amiibo_data, "name"), this->valid = false;);
        EMU_DO_UNLESS(this->HasKey(this->amiibo_data, "id"), this->valid = false;);
    }

    std::string VirtualAmiibo::GetName() {
        return this->ReadPlain<std::string>(this->amiibo_data, "name");
    }

    void VirtualAmiibo::SetName(const std::string &name) {
        this->WritePlain(this->amiibo_data, "name", name);
    }

    AmiiboUuidInfo VirtualAmiibo::GetUuidInfo() {
        AmiiboUuidInfo info = {};
        const bool has_uuid = this->amiibo_data.find("uuid") != this->amiibo_data.end();
        info.random_uuid = !has_uuid;
        if(has_uuid) {
            this->ReadByteArray(info.uuid, "uuid");
        }
        return info;
    }

    void VirtualAmiibo::SetUuidInfo(AmiiboUuidInfo info) {
        this->amiibo_data.erase("uuid");
        if(!info.random_uuid) {
            this->WriteByteArray(info.uuid, 10, "uuid");
        }
    }

    AmiiboId VirtualAmiibo::GetAmiiboId() {
        AmiiboId id = {};
        if(this->HasKey(this->amiibo_data, "id")) {
            auto id_obj = this->amiibo_data["id"];
            id.character_id.game_character_id = this->ReadPlain<u16>(id_obj, "game_character_id");
            id.character_id.character_variant = this->ReadPlain<u8>(id_obj, "character_variant");
            id.series = this->ReadPlain<u8>(id_obj, "series");
            id.model_number = this->ReadPlain<u16>(id_obj, "model_number");
            id.figure_type = this->ReadPlain<u8>(id_obj, "figure_type");
        }
        return id;
    }

    void VirtualAmiibo::SetAmiiboId(AmiiboId id) {
        auto id_obj = JSON::object();
        this->WritePlain(id_obj, "game_character_id", id.character_id.game_character_id);
        this->WritePlain(id_obj, "character_variant", id.character_id.character_variant);
        this->WritePlain(id_obj, "series", id.series);
        this->WritePlain(id_obj, "model_number", id.model_number);
        this->WritePlain(id_obj, "figure_type", id.figure_type);
        this->amiibo_data["id"] = id_obj;
    }

    std::string VirtualAmiibo::GetMiiCharInfoFileName() {
        return this->ReadPlain<std::string>(this->amiibo_data, "mii_charinfo_file");
    }

    void VirtualAmiibo::SetMiiCharInfoFileName(const std::string &char_info_path) {
        this->WritePlain(this->amiibo_data, "mii_charinfo_file", char_info_path);
    }

    Date VirtualAmiibo::GetFirstWriteDate() {
        return this->ReadDate("first_write_date");
    }

    void VirtualAmiibo::SetFirstWriteDate(Date date) {
        this->WriteDate("first_write_date", date);
    }

    Date VirtualAmiibo::GetLastWriteDate() {
        return this->ReadDate("last_write_date");
    }

    void VirtualAmiibo::SetLastWriteDate(Date date) {
        this->WriteDate("last_write_date", date);
    }

    u16 VirtualAmiibo::GetWriteCounter() {
        return this->ReadPlain<u16>(this->amiibo_data, "write_counter");
    }

    void VirtualAmiibo::SetWriteCounter(u16 counter) {
        this->WritePlain(this->amiibo_data, "write_counter", counter);
    }

    void VirtualAmiibo::NotifyWritten() {
        // Update counter, if 0xFFFF it won't be updated anymore (this is what N does)
        auto counter = this->GetWriteCounter();
        if(counter < UINT16_MAX) {
            counter++;
        }
        this->SetWriteCounter(counter);

        Date cur_date = this->MakeCurrentDate();
        this->SetLastWriteDate(cur_date);

        this->Save();
    }

    u32 VirtualAmiibo::GetVersion() {
        return this->ReadPlain<u32>(this->amiibo_data, "version");
    }

    void VirtualAmiibo::SetVersion(u32 version) {
        this->WritePlain(this->amiibo_data, "version", version);
    }

    void VirtualAmiibo::FullyRemove() {
        fs::DeleteDirectory(this->path);
    }

    TagInfo VirtualAmiibo::ProduceTagInfo() {
        TagInfo info = {};
        // Normally amiibos have 7 here (3 trailing zeros), but this doesn't seem to be enforced/checked
        info.info.uuid_length = 10;
        auto uuid_info = this->GetUuidInfo();
        if(uuid_info.random_uuid) {
            // Random UUID can be helpful for amiibos used for daily bonus stuff - meaning infinite supply with some games like BOTW
            // Follow most amiibos' pattern, and zero the last 3 bytes
            randomGet(info.info.uuid, 7);
            info.info.uuid[7] = 0;
            info.info.uuid[8] = 0;
            info.info.uuid[9] = 0;
        }
        else {
            memcpy(info.info.uuid, uuid_info.uuid, 10);
        }
        
        info.info.tag_type = VirtualAmiibo::DefaultTagType;
        info.info.protocol = VirtualAmiibo::DefaultProtocol;

        return info;
    }

    RegisterInfo VirtualAmiibo::ProduceRegisterInfo() {
        RegisterInfo info = {};
        auto charinfo = this->ReadMiiCharInfo();
        memcpy(&info.info.mii, &charinfo, sizeof(charinfo));

        auto first_w_date = this->GetFirstWriteDate();
        info.info.first_write_date = first_w_date;
        
        auto name = this->GetName();
        strncpy(info.info.name, name.c_str(), RegisterInfoImpl::AmiiboNameLength);

        return info;
    }

    ModelInfo VirtualAmiibo::ProduceModelInfo() {
        ModelInfo info = {};
        info.info.id = this->GetAmiiboId();
        EMU_LOG_FMT("Processed amiibo ID { Game & character ID: " << info.info.id.character_id.game_character_id << ", Character variant: " << (int)info.info.id.character_id.character_variant << ", Figure type: " << (int)info.info.id.figure_type << ", Model number: " << info.info.id.model_number << ", Series: " << (int)info.info.id.series << " }")
        return info;
    }

    CommonInfo VirtualAmiibo::ProduceCommonInfo() {
        CommonInfo info = {};
        auto last_w_date = this->GetLastWriteDate();
        info.info.last_write_year = last_w_date.year;
        info.info.last_write_month = last_w_date.month;
        info.info.last_write_day = last_w_date.day;

        auto w_counter = this->GetWriteCounter();
        info.info.write_counter = w_counter;

        auto ver = this->GetVersion();
        info.info.version = ver;
        info.info.application_area_size = AreaManager::DefaultSize;

        return info;
    }

    VirtualAmiiboData VirtualAmiibo::ProduceData() {
        VirtualAmiiboData data = {};
        data.uuid = this->GetUuidInfo();
        auto name = this->GetName();
        strncpy(data.name, name.c_str(), 40);
        data.first_write_date = this->GetFirstWriteDate();
        data.last_write_date = this->GetLastWriteDate();
        data.mii_charinfo = this->ReadMiiCharInfo();
        return data;
    }

    VirtualAmiiboV3::VirtualAmiiboV3(const std::string &amiibo_dir) : IVirtualAmiiboBase(amiibo_dir) {
        this->tag_data = fs::LoadJSONFile(this->GetJSONFileName("tag"));
        this->register_data = fs::LoadJSONFile(this->GetJSONFileName("register"));
        this->common_data = fs::LoadJSONFile(this->GetJSONFileName("common"));
        this->model_data = fs::LoadJSONFile(this->GetJSONFileName("model"));
        // Some checks to see if the amiibo is correct
        EMU_DO_UNLESS(!this->tag_data.empty(), this->valid = false;);
        EMU_DO_UNLESS(!this->register_data.empty(), this->valid = false;);
        EMU_DO_UNLESS(!this->common_data.empty(), this->valid = false;);
        EMU_DO_UNLESS(!this->model_data.empty(), this->valid = false;);
        EMU_DO_UNLESS(this->HasKey(this->register_data, "name"), this->valid = false;);
        EMU_DO_UNLESS(this->HasKey(this->model_data, "amiiboId"), this->valid = false;);
        EMU_DO_UNLESS(this->HasKey(this->tag_data, "uuid") || this->HasKey(this->tag_data, "randomUuid"), this->valid = false;);
    }

    std::string VirtualAmiiboV3::GetName() {
        return this->ReadPlain<std::string>(this->register_data, "name");
    }

    AmiiboUuidInfo VirtualAmiiboV3::GetUuidInfo() {
        AmiiboUuidInfo info = {};
        info.random_uuid = this->tag_data.value("randomUuid", false);
        if(!info.random_uuid) {
            this->ReadStringByteArray(this->tag_data, info.uuid, "uuid");
        }
        return info;
    }

    AmiiboId VirtualAmiiboV3::GetAmiiboId() {
        u8 array[8] = {0};
        this->ReadStringByteArray(this->model_data, array, "amiiboId");

        auto old_id = *(OldAmiiboId*)array;
        // Reverse model number field (BE)
        old_id.model_number = __builtin_bswap16(old_id.model_number);
        
        auto id = AmiiboId::FromOldAmiiboId(old_id);
        return id;
    }

    std::string VirtualAmiiboV3::GetMiiCharInfoFileName() {
        return this->ReadPlain<std::string>(this->register_data, "miiCharInfo");
    }

    Date VirtualAmiiboV3::GetFirstWriteDate() {
        return this->ReadStringDate(this->register_data, "firstWriteDate");
    }

    Date VirtualAmiiboV3::GetLastWriteDate() {
        return this->ReadStringDate(this->common_data, "lastWriteDate");
    }

    u16 VirtualAmiiboV3::GetWriteCounter() {
        return this->ReadPlain<u16>(this->common_data, "writeCounter");
    }

    u32 VirtualAmiiboV3::GetVersion() {
        return this->ReadPlain<u32>(this->common_data, "version");
    }

    void VirtualAmiiboV3::FullyRemove() {
        fs::DeleteDirectory(this->path);
    }

    VirtualBinAmiibo::VirtualBinAmiibo(const std::string &path) : IVirtualAmiiboBase(path) {
        if(fs::GetFileSize(this->path) < sizeof(RawAmiibo)) {
            this->valid = false;
            return;
        }
        this->raw_data = fs::Read<RawAmiibo>(this->path);
        this->base_date = this->MakeCurrentDate();
    }

    std::string VirtualBinAmiibo::GetName() {
        return fs::RemoveExtension(fs::GetBaseName(this->path));
    }

    AmiiboUuidInfo VirtualBinAmiibo::GetUuidInfo() {
        AmiiboUuidInfo info = {};
        info.random_uuid = false;
        memcpy(info.uuid, this->raw_data.uuid, 10);
        return info;
    }

    AmiiboId VirtualBinAmiibo::GetAmiiboId() {
        u8 id_array[8] = {0};
        memcpy(id_array, this->raw_data.amiibo_id, 8);
        auto old_id = *(OldAmiiboId*)id_array;
        // Reverse model number field (BE)
        old_id.model_number = __builtin_bswap16(old_id.model_number);
        
        auto id = AmiiboId::FromOldAmiiboId(old_id);
        return id;
    }

    std::string VirtualBinAmiibo::GetMiiCharInfoFileName() {
        return "";
    }

    Date VirtualBinAmiibo::GetFirstWriteDate() {
        return this->base_date;
    }

    Date VirtualBinAmiibo::GetLastWriteDate() {
        return this->base_date;
    }

    u16 VirtualBinAmiibo::GetWriteCounter() {
        return 0;
    }

    u32 VirtualBinAmiibo::GetVersion() {
        return 0;
    }

    void VirtualBinAmiibo::FullyRemove() {
        fs::DeleteFile(this->path);
    }

}