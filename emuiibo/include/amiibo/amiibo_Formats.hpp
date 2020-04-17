
#pragma once
#include <emu_Types.hpp>
#include <amiibo/amiibo_Areas.hpp>
#include <ctime>

namespace amiibo {

    struct AmiiboUuidInfo {
        bool random_uuid;
        u8 uuid[10];
    };

    // This is the amiibo data sent by IPC

    struct VirtualAmiiboData : public ams::sf::LargeData {
        AmiiboUuidInfo uuid;
        char name[40 + 1];
        Date first_write_date;
        Date last_write_date;
        MiiCharInfo mii_charinfo;

        inline bool IsValid() {
            return strlen(this->name);
        }

    };

    class IVirtualAmiiboBase {

        protected:
            std::string path;
            bool valid;

            template<typename T>
            inline T ReadPlain(JSON &json, const std::string &key) {
                return json.value<T>(key, T());
            }

            template<typename T>
            inline void WritePlain(JSON &json, const std::string &key, T t) {
                json[key] = t;
            }

            inline bool HasKey(JSON &json, const std::string &key) {
                return json.count(key);
            }

            inline Date MakeCurrentDate() {
                Date cur_date = {};
                auto cur_time = std::time(nullptr);
                auto cur_time_local = std::localtime(&cur_time);
                cur_date.year = cur_time_local->tm_year + 1900;
                cur_date.month = cur_time_local->tm_mon + 1;
                cur_date.day = cur_time_local->tm_mday;
                return cur_date;
            }

            static inline MiiCharInfo GenerateRandomMii() {
                MiiCharInfo charinfo = {};
                MiiDatabase db;
                auto rc = miiOpenDatabase(&db, MiiSpecialKeyCode_Normal);
                if(R_SUCCEEDED(rc)) {
                    miiDatabaseBuildRandom(&db, MiiAge_All, MiiGender_All, MiiFaceColor_All, &charinfo);
                    miiDatabaseClose(&db);
                }
                return charinfo;
            }

        public:
            IVirtualAmiiboBase() : valid(false) {}

            IVirtualAmiiboBase(const std::string &amiibo_path) : path(amiibo_path), valid(true) {}

            virtual std::string GetName() = 0;

            virtual AmiiboUuidInfo GetUuidInfo() = 0;

            virtual AmiiboId GetAmiiboId() = 0;

            virtual std::string GetMiiCharInfoFileName() = 0;

            virtual Date GetFirstWriteDate() = 0;

            virtual Date GetLastWriteDate() = 0;

            virtual u16 GetWriteCounter() = 0;

            virtual u32 GetVersion() = 0;

            virtual void FullyRemove() = 0;

            inline std::string GetPath() {
                return this->path;
            }

            inline std::string GetMiiCharInfoPath() {
                // This won't be used for raw bin amiibo format, plus it wouldn't be valid since path is a file :P
                return fs::Concat(this->path, this->GetMiiCharInfoFileName());
            }

            inline MiiCharInfo ReadMiiCharInfo() {
                MiiCharInfo charinfo = {};
                auto charinfo_path = this->GetMiiCharInfoPath();
                if(fs::IsFile(charinfo_path)) {
                    charinfo = fs::Read<MiiCharInfo>(charinfo_path);
                }
                else {
                    // The amiibo has no mii charinfo data
                    // This might be a new emutool amiibo which needs a mii
                    // Let's generate a random mii then
                    charinfo = GenerateRandomMii();
                    // Save it too, for the next time
                    fs::Save(charinfo_path, charinfo);
                }
                return charinfo;
            }

            inline bool IsValid() {
                return this->valid;
            }

    };

    // Old formats supported by emuiibo (how are they named here):
    // - Bin (raw binaries, always supported but mainly used in emuiibo 0.1)
    // - V3 (format used in emuiibo 0.3.x and 0.4)

    class VirtualBinAmiibo;
    class VirtualAmiiboV3;

    class VirtualAmiibo : public IVirtualAmiiboBase {

        friend class IVirtualAmiiboBase;

        public:
            static inline constexpr u32 DefaultProtocol = UINT32_MAX;
            static inline constexpr u32 DefaultTagType = UINT32_MAX;

        private:
            JSON amiibo_data;
            AreaManager area_manager;

            inline void ReadByteArray(u8 *out_arr, const std::string &key) {
                auto array = this->amiibo_data[key];
                for(u32 i = 0; i < array.size(); i++) {
                    auto item = array[i];
                    auto value = item.get<u32>();
                    auto byte = (u8)(value & 0xff);
                    out_arr[i] = byte;
                }
            }

            inline void WriteByteArray(u8 *arr, u32 len, const std::string &key) {
                auto array = JSON::array();
                for(u32 i = 0; i < len; i++) {
                    auto byte = (u32)arr[i];
                    array[i] = byte;
                }
                this->amiibo_data[key] = array;
            }

            inline Date ReadDate(const std::string &key) {
                Date date = {};
                auto date_item = this->amiibo_data[key];
                auto y_item = date_item["y"];
                auto m_item = date_item["m"];
                auto d_item = date_item["d"];
                date.year = y_item.get<u16>();
                date.month = m_item.get<u8>();
                date.day = m_item.get<u8>();
                return date;
            }

            inline void WriteDate(const std::string &key, Date date) {
                auto date_obj = JSON::object();
                date_obj["y"] = date.year;
                date_obj["m"] = date.month;
                date_obj["d"] = date.day;
                this->amiibo_data[key] = date_obj;
            }

        public:
            VirtualAmiibo() : IVirtualAmiiboBase() {}

            VirtualAmiibo(const std::string &amiibo_dir);

            std::string GetName() override;
            void SetName(const std::string &name);

            AmiiboUuidInfo GetUuidInfo() override;
            void SetUuidInfo(AmiiboUuidInfo info);

            AmiiboId GetAmiiboId() override;
            void SetAmiiboId(AmiiboId id);

            std::string GetMiiCharInfoFileName() override;
            void SetMiiCharInfoFileName(const std::string &char_info_name);

            Date GetFirstWriteDate() override;
            void SetFirstWriteDate(Date date);

            Date GetLastWriteDate() override;
            void SetLastWriteDate(Date date);

            u16 GetWriteCounter() override;
            void SetWriteCounter(u16 counter);
            // Increases the write counter
            void NotifyWritten();

            void Save();

            u32 GetVersion() override;
            void SetVersion(u32 version);

            void FullyRemove() override;

            TagInfo ProduceTagInfo();
            RegisterInfo ProduceRegisterInfo();
            ModelInfo ProduceModelInfo();
            CommonInfo ProduceCommonInfo();

            VirtualAmiiboData ProduceData();

            inline AreaManager &GetAreaManager() {
                return this->area_manager;
            }

            template<typename V>
            static inline constexpr bool HasMiiCharInfo() {
                static_assert(std::is_base_of_v<IVirtualAmiiboBase, V>, "Invalid amiibo type");
                // All virtual amiibo formats have a mii charinfo file except for raw bin amiibos
                if constexpr(std::is_same_v<V, VirtualBinAmiibo>) {
                    return false;
                }
                return true;
            }

            template<typename V>
            static inline bool ConvertVirtualAmiibo(const std::string &path) {
                static_assert(std::is_base_of_v<IVirtualAmiiboBase, V>, "Invalid amiibo type");
                V old_amiibo(path);
                VirtualAmiibo amiibo;
                if(!old_amiibo.IsValid()) {
                    return false;
                }
                amiibo.SetName(old_amiibo.GetName());
                amiibo.SetUuidInfo(old_amiibo.GetUuidInfo());
                amiibo.SetAmiiboId(old_amiibo.GetAmiiboId());
                amiibo.SetFirstWriteDate(old_amiibo.GetFirstWriteDate());
                amiibo.SetLastWriteDate(old_amiibo.GetLastWriteDate());
                amiibo.SetWriteCounter(old_amiibo.GetWriteCounter());
                amiibo.SetVersion(old_amiibo.GetVersion());
                auto has_charinfo = HasMiiCharInfo<V>();
                if(has_charinfo) {
                    // If the mii file path is invalid, we must create a new mii
                    // This should (only?) happen if a virtual amiibo is not properly generated or is corrupted...?
                    has_charinfo = fs::IsFile(old_amiibo.GetMiiCharInfoPath());
                }
                MiiCharInfo charinfo = {};
                if(has_charinfo) {
                    charinfo = old_amiibo.ReadMiiCharInfo();
                    amiibo.SetMiiCharInfoFileName(old_amiibo.GetMiiCharInfoFileName());
                }
                else {
                    // Default mii charinfo file name
                    amiibo.SetMiiCharInfoFileName("mii-charinfo.bin");
                    // Generate a random mii if the original virtual amiibo doesn't have one
                    // Otherwise, the charinfo struct should already be populated
                    charinfo = GenerateRandomMii();
                }
                // Manually indicate the amiibo is valid
                amiibo.path = old_amiibo.GetPath();
                amiibo.valid = true;
                auto areas_path = fs::Concat(old_amiibo.GetPath(), "areas");
                const bool has_areas = fs::IsDirectory(areas_path);
                if(has_areas) {
                    // Move areas to a temp location
                    fs::RecreateDirectory(consts::TempConversionAreasDir);
                    FS_FOR(areas_path, entry, area_path, {
                        if(fs::MatchesExtension(area_path, "bin")) {
                            auto out_path = fs::Concat(consts::TempConversionAreasDir, entry);
                            fs::CopyFile(area_path, out_path);
                        }
                    });
                }
                old_amiibo.FullyRemove();
                amiibo.Save();
                // After creating the new amiibo's layout, save the mii
                fs::Save(amiibo.GetMiiCharInfoPath(), charinfo);
                if(has_areas) {
                    fs::CreateDirectory(areas_path);
                    // Move temp areas back to the new amiibo's dir
                    FS_FOR(consts::TempConversionAreasDir, entry, area_path, {
                        auto out_path = fs::Concat(areas_path, entry);
                        fs::CopyFile(area_path, out_path);
                    });
                    fs::RecreateDirectory(consts::TempConversionAreasDir);
                }
                return true;
            }

            // This just checks if the files needed are present

            template<typename V>
            static inline bool IsValidVirtualAmiiboImpl(const std::string &amiibo_path) {
                static_assert(std::is_base_of_v<IVirtualAmiiboBase, V>, "Invalid amiibo type");
                if constexpr(std::is_same_v<V, VirtualBinAmiibo>) {
                    // Just check if it's a file and ends in .bin :P
                    return fs::IsFile(amiibo_path) && fs::MatchesExtension(amiibo_path, "bin");
                }
                else if constexpr(std::is_same_v<V, VirtualAmiiboV3>) {
                    // The V3 format was made of tag.json, model.json, common.json and register.json files
                    return fs::IsDirectory(amiibo_path) && fs::IsFile(fs::Concat(amiibo_path, "tag.json")) && fs::IsFile(fs::Concat(amiibo_path, "common.json")) && fs::IsFile(fs::Concat(amiibo_path, "model.json")) && fs::IsFile(fs::Concat(amiibo_path, "register.json"));
                }
                else if constexpr(std::is_same_v<V, VirtualAmiibo>) {
                    // The current format has an amiibo.flag file in case anyone would like to enable/disable a virtual amiibo from being recognized or used by emuiibo
                    return fs::IsDirectory(amiibo_path) && fs::IsFile(fs::Concat(amiibo_path, "amiibo.json")) && fs::IsFile(fs::Concat(amiibo_path, "amiibo.flag"));
                }
                return false;
            }

            // This two also validate that the amiibo contents are correct

            template<typename V>
            static inline bool IsValidVirtualAmiiboType(const std::string &amiibo_path) {
                static_assert(std::is_base_of_v<IVirtualAmiiboBase, V>, "Invalid amiibo type");
                if(IsValidVirtualAmiiboImpl<V>(amiibo_path)) {
                    V amiibo(amiibo_path);
                    return amiibo.IsValid();
                }
                return false;
            }

            static inline bool IsValidVirtualAmiibo(const std::string &amiibo_path) {
                return IsValidVirtualAmiiboType<VirtualAmiibo>(amiibo_path);
            }

            static inline bool GetValidVirtualAmiibo(const std::string &amiibo_path, VirtualAmiibo &out_amiibo) {
                if(IsValidVirtualAmiiboImpl<VirtualAmiibo>(amiibo_path)) {
                    out_amiibo = VirtualAmiibo(amiibo_path);
                    return out_amiibo.IsValid();
                }
                return false;
            }

    };

    // 0.3.x/0.4 virtual amiibo format

    class VirtualAmiiboV3 : public IVirtualAmiiboBase {

        private:
            JSON tag_data;
            JSON register_data;
            JSON common_data;
            JSON model_data;

            inline std::string GetJSONFileName(const std::string &name) {
                return fs::Concat(this->path, name + ".json");
            }

            inline void ReadStringByteArray(JSON &json, u8 *out_arr, const std::string &key) {
                auto array = this->ReadPlain<std::string>(json, key);
                if(!array.empty()) {
                    std::stringstream strm;
                    for(u32 i = 0; i < array.length(); i += 2) {
                        strm << std::hex << array.substr(i, 2);
                        u32 tmpbyte = 0;
                        strm >> tmpbyte;
                        out_arr[i / 2] = (u8)(tmpbyte & 0xff);
                        strm.str("");
                        strm.clear();
                    }
                }
            }

            inline Date ReadStringDate(JSON &json, const std::string &key) {
                auto date_str = this->ReadPlain<std::string>(json, key);
                auto date_year_str = date_str.substr(0, 4);
                auto date_month_str = date_str.substr(5, 2);
                auto date_day_str = date_str.substr(8, 2);
                Date date = {};
                date.year = (u16)std::stoi(date_year_str);
                date.month = (u8)std::stoi(date_month_str);
                date.day = (u8)std::stoi(date_day_str);
                return date;
            }

        public:
            VirtualAmiiboV3(const std::string &amiibo_dir);

            std::string GetName() override;

            AmiiboUuidInfo GetUuidInfo() override;

            AmiiboId GetAmiiboId() override;

            std::string GetMiiCharInfoFileName() override;

            Date GetFirstWriteDate() override;

            Date GetLastWriteDate() override;

            u16 GetWriteCounter() override;

            u32 GetVersion() override;

            void FullyRemove() override;

    };

    // Raw binary, 0.1 virtual amiibo format

    struct RawAmiibo {
        u8 uuid[10];
        u8 unk1[6];
        u8 unk2[1];
        u16 unk_counter;
        u8 unk3;
        u8 unk_crypto[0x40];
        u8 amiibo_id[8];
    } PACKED;

    class VirtualBinAmiibo : public IVirtualAmiiboBase {

        private:
            RawAmiibo raw_data;
            Date base_date;

        public:
            VirtualBinAmiibo() : IVirtualAmiiboBase() {}
            
            VirtualBinAmiibo(const std::string &path);

            std::string GetName() override;

            AmiiboUuidInfo GetUuidInfo() override;

            AmiiboId GetAmiiboId() override;

            std::string GetMiiCharInfoFileName() override;

            Date GetFirstWriteDate() override;

            Date GetLastWriteDate() override;

            u16 GetWriteCounter() override;

            u32 GetVersion() override;

            void FullyRemove() override;

    };

}