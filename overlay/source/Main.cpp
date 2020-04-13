
#define TESLA_INIT_IMPL
#include <emuiibo.hpp>
#include <libtesla_ext.hpp>
#include <dirent.h>
#include <filesystem>

namespace {

    bool g_emuiibo_init_ok = false;
    bool g_category_list_update_flag = false;
    bool g_current_app_intercepted = false;
    char g_emuiibo_amiibo_dir[FS_MAX_PATH];
    emu::Version g_emuiibo_version;

    char g_active_amiibo_path[FS_MAX_PATH];
    emu::VirtualAmiiboData g_active_amiibo_data;

    inline bool IsActiveAmiiboValid() {
        return strlen(g_active_amiibo_path) > 0;
    }

    inline void UpdateActiveAmiibo() {
        emu::GetActiveVirtualAmiibo(&g_active_amiibo_data, g_active_amiibo_path, FS_MAX_PATH);
    }

    // Returns true if the value changed
    inline bool UpdateCurrentApplicationIntercepted() {
        bool ret = false;
        emu::IsCurrentApplicationIdIntercepted(&ret);
        if(ret != g_current_app_intercepted) {
            g_current_app_intercepted = ret;
            return true;
        }
        return false;
    }

    inline std::string MakeActiveAmiiboText() {
        if(IsActiveAmiiboValid()) {
            return g_active_amiibo_data.name;
        }
        return "No active virtual amiibo";
    }

    inline std::string MakeTitleText() {
        if(!g_emuiibo_init_ok) {
            return "emuiibo";
        }
        return "emuiibo v" + std::to_string(g_emuiibo_version.major) + "." + std::to_string(g_emuiibo_version.minor) + "." + std::to_string(g_emuiibo_version.micro) + " (" + (g_emuiibo_version.dev_build ? "dev" : "release") + ")";
    }

    inline std::string MakeStatusText() {
        if(!g_emuiibo_init_ok) {
            return "emuiibo was not accessed.";
        }
        std::string msg = "Emulation: ";
        auto e_status = emu::GetEmulationStatus();
        switch(e_status) {
            case emu::EmulationStatus::On: {
                msg += "on\n";
                auto v_status = emu::GetActiveVirtualAmiiboStatus();
                switch(v_status) {
                    case emu::VirtualAmiiboStatus::Invalid: {
                        msg += "No active virtual amiibo.";
                        break;
                    }
                    case emu::VirtualAmiiboStatus::Connected: {
                        msg += "Virtual amiibo: ";
                        msg += g_active_amiibo_data.name;
                        msg += " (connected - select to disconnect)";
                        break;
                    }
                    case emu::VirtualAmiiboStatus::Disconnected: {
                        msg += "Virtual amiibo: ";
                        msg += g_active_amiibo_data.name;
                        msg += " (disconnected - select to connect)";
                        break;
                    }
                }
                msg += "\n";
                if(g_current_app_intercepted) {
                    msg += "Current game is being intercepted by emuiibo.";
                }
                else {
                    msg += "Current game is not being intercepted.";
                }
                break;
            }
            case emu::EmulationStatus::Off: {
                msg += "off";
                break;
            }
        }
        return msg;
    }

    inline std::string MakeGameInterceptedText() {
        std::string msg = "";
        if(g_current_app_intercepted) {
            msg += "intercepted";
        }
        else {
            msg += "not intercepted.";
        }
        return msg;
    }

    inline std::string MakeActiveAmiiboStatusText() {
        std::string msg = "";
        if(IsActiveAmiiboValid()) {
            auto v_status = emu::GetActiveVirtualAmiiboStatus();
            switch(v_status) {
                case emu::VirtualAmiiboStatus::Invalid: {
                    msg = "";
                    break;
                }
                case emu::VirtualAmiiboStatus::Connected: {
                    msg = "connected";
                    break;
                }
                case emu::VirtualAmiiboStatus::Disconnected: {
                    msg = "disconnected";
                    break;
                }
            }
        }
        return msg;
    }

    inline std::string MakeCategoryText(std::string category_path) {
        std::string msg = "";
        std::filesystem::path path = category_path;
        msg = path.filename();
        return msg;
    }

}

class AmiiboList : public tsl::Gui {

    private:
        tsl::elm::DoubleSectionOverlayFrame *root_frame;
        tsl::elm::BigCategoryHeader *category_header;
        tsl::elm::SmallListItem *game_header = new tsl::elm::SmallListItem("Current game is");
        tsl::elm::SmallListItem *count_item;
        tsl::elm::SmallListItem *amiibo_header;
        tsl::elm::List *top_list;
        tsl::elm::List *bottom_list;
        std::string amiibo_path;

    public:
        AmiiboList(const std::string &path) : root_frame(new tsl::elm::DoubleSectionOverlayFrame(MakeTitleText(), "", tsl::SectionsLayout::same, true)), amiibo_path(path) {}

        bool OnItemClick(u64 keys, const std::string &path) {
            if(keys & KEY_A) {
                char amiibo_path[FS_MAX_PATH] = {0};
                strcpy(amiibo_path, path.c_str());
                // Set active amiibo and update our active amiibo value
                emu::SetActiveVirtualAmiibo(amiibo_path, FS_MAX_PATH);
                UpdateActiveAmiibo();
                amiibo_header->setText(MakeActiveAmiiboText());
                return true;   
            }
            return false;
        }

        virtual tsl::elm::Element *createUI() override {
            top_list = new tsl::elm::List();
            bottom_list = new tsl::elm::List();
            amiibo_header = new tsl::elm::SmallListItem(MakeActiveAmiiboText());
            amiibo_header->setColoredValue(MakeActiveAmiiboStatusText(), emu::GetActiveVirtualAmiiboStatus()==emu::VirtualAmiiboStatus::Disconnected ? tsl::style::color::ColorWarning : tsl::style::color::ColorHighlight);
            category_header = new tsl::elm::BigCategoryHeader(MakeCategoryText(amiibo_path)); 
            count_item = new tsl::elm::SmallListItem("available virtual amiibos");

            u32 count = 0;
            tsl::hlp::doWithSDCardHandle([&](){
                auto dir = opendir(this->amiibo_path.c_str());
                if(dir) {
                    while(true) {
                        auto entry = readdir(dir);
                        if(entry == nullptr) {
                            break;
                        }
                        char path[FS_MAX_PATH] = {0};
                        auto str_path = this->amiibo_path + "/" + entry->d_name;
                        strcpy(path, str_path.c_str());
                        // Find virtual amiibo
                        emu::VirtualAmiiboData data = {};
                        if(R_SUCCEEDED(emu::TryParseVirtualAmiibo(path, FS_MAX_PATH, &data))) {
                            auto item = new tsl::elm::SmallListItem(data.name);
                            item->setClickListener(std::bind(&AmiiboList::OnItemClick, this, std::placeholders::_1, str_path));
                            bottom_list->addItem(item);
                            count++;
                        }
                    }
                    closedir(dir);
                }
            });

            top_list->addItem(new tsl::elm::CategoryHeader("emulation status"));
            top_list->addItem(game_header);
            top_list->addItem(amiibo_header);

            top_list->addItem(category_header);

            count_item->setValue(std::to_string(count), true);
            top_list->addItem(count_item);

            root_frame->setTopSection(top_list);
            root_frame->setBottomSection(bottom_list);
            return root_frame;
        }

        virtual void update() override {
            UpdateCurrentApplicationIntercepted();
            this->game_header->setColoredValue(MakeGameInterceptedText(), g_current_app_intercepted ? tsl::style::color::ColorHighlight : tsl::style::color::ColorWarning);
        }

};

class MainGui : public tsl::Gui {

    private:
        tsl::elm::NamedStepTrackBar *toggle_item = new tsl::elm::NamedStepTrackBar("\u22EF", { "Off", "On" });
        tsl::elm::SmallListItem *game_header = new tsl::elm::SmallListItem("Current game is");
        tsl::elm::SmallListItem *amiibo_header;
        tsl::elm::SmallListItem *category_header;
        tsl::elm::DoubleSectionOverlayFrame *root_frame;
        
    public:
        MainGui() : amiibo_header(new tsl::elm::SmallListItem(MakeActiveAmiiboText())), category_header(new tsl::elm::SmallListItem("Categories")), root_frame(new tsl::elm::DoubleSectionOverlayFrame(MakeTitleText(), "", tsl::SectionsLayout::same, true)) {}

        void Refresh() {
            this->game_header->setColoredValue(MakeGameInterceptedText(), g_current_app_intercepted ? tsl::style::color::ColorHighlight : tsl::style::color::ColorWarning);
            this->amiibo_header->setText(MakeActiveAmiiboText());
            this->amiibo_header->setColoredValue(MakeActiveAmiiboStatusText(), emu::GetActiveVirtualAmiiboStatus()==emu::VirtualAmiiboStatus::Disconnected ? tsl::style::color::ColorWarning : tsl::style::color::ColorHighlight);
            u8 toggle_progress;
            switch(emu::GetEmulationStatus()) {
                case emu::EmulationStatus::On:
                    toggle_progress = 1;
                    break;
                case emu::EmulationStatus::Off:
                    toggle_progress = 0;
                    break;
            }
            this->toggle_item->setProgress(toggle_progress);
        }

        static bool OnItemClick(u64 keys, const std::string &path) {
            if(keys & KEY_A) {
                tsl::changeTo<AmiiboList>(path);
                g_category_list_update_flag = true;
                return true;
            }
            return false;
        }

        bool OnAmiiboHeaderClick(u64 keys, const std::string &path) {
            if(keys & KEY_A) {
                char amiibo_path[FS_MAX_PATH] = {0};
                strcpy(amiibo_path, path.c_str());
                if(IsActiveAmiiboValid()) {
                    if(strcmp(g_active_amiibo_path, amiibo_path) == 0) {
                        // User selected the active amiibo, so let's change connection then
                        auto status = emu::GetActiveVirtualAmiiboStatus();
                        switch(status) {
                            case emu::VirtualAmiiboStatus::Connected: {
                                emu::SetActiveVirtualAmiiboStatus(emu::VirtualAmiiboStatus::Disconnected);
                                root_frame->setSubtitle(MakeStatusText());
                                break;
                            }
                            case emu::VirtualAmiiboStatus::Disconnected: {
                                emu::SetActiveVirtualAmiiboStatus(emu::VirtualAmiiboStatus::Connected);
                                root_frame->setSubtitle(MakeStatusText());
                                break;
                            }
                            default:
                                break;
                        }
                        return true;
                    }
                }
                return true;   
            }
            return false;
        }


        virtual tsl::elm::Element *createUI() override {
            auto top_list = new tsl::elm::List();
            auto bottom_list = new tsl::elm::List();
            
            if(g_emuiibo_init_ok) {
                
                toggle_item->setValueChangedListener([&](u8 progress) {
                    switch(progress) {
                        case 1: {
                            emu::SetEmulationStatus(emu::EmulationStatus::On);
                            break;
                        }
                        case 0: {
                            emu::SetEmulationStatus(emu::EmulationStatus::Off);
                            break;
                        }
                    }    
                });

                /*
                u8 toggle_progress;
                switch(emu::GetEmulationStatus()) {
                    case emu::EmulationStatus::On:
                        toggle_progress = 1;
                        break;
                    case emu::EmulationStatus::Off:
                        toggle_progress = 0;
                        break;
                }
                toggle_item->setProgress(toggle_progress);

                game_header->setColoredValue(MakeGameInterceptedText(), g_current_app_intercepted ? tsl::style::color::ColorHighlight : tsl::style::color::ColorWarning);

                amiibo_header->setText(MakeActiveAmiiboText());
                amiibo_header->setColoredValue(MakeActiveAmiiboStatusText(), emu::GetActiveVirtualAmiiboStatus()==emu::VirtualAmiiboStatus::Disconnected ? tsl::style::color::ColorWarning : tsl::style::color::ColorHighlight);
                */
                amiibo_header->setClickListener(std::bind(&MainGui::OnAmiiboHeaderClick, this, std::placeholders::_1, g_emuiibo_amiibo_dir));

                // Root
                auto root_item = new tsl::elm::SmallListItem("<root>");
                root_item->setClickListener(std::bind(&MainGui::OnItemClick, std::placeholders::_1, g_emuiibo_amiibo_dir));
                bottom_list->addItem(root_item);

                u32 count = 1; // Root
                tsl::hlp::doWithSDCardHandle([&](){
                    auto dir = opendir(g_emuiibo_amiibo_dir);
                    if(dir) {
                        while(true) {
                            auto entry = readdir(dir);
                            if(entry == nullptr) {
                                break;
                            }
                            char path[FS_MAX_PATH] = {0};
                            auto str_path = std::string(g_emuiibo_amiibo_dir) + "/" + entry->d_name;
                            strcpy(path, str_path.c_str());
                            // If it's a valid amiibo, skip
                            emu::VirtualAmiiboData tmp_data;
                            if(R_SUCCEEDED(emu::TryParseVirtualAmiibo(path, FS_MAX_PATH, &tmp_data))) {
                                continue;
                            }
                            if(entry->d_type & DT_DIR) {
                                auto item = new tsl::elm::SmallListItem(entry->d_name);
                                item->setClickListener(std::bind(&MainGui::OnItemClick, std::placeholders::_1, str_path));
                                bottom_list->addItem(item);
                                count++;
                            }
                        }
                        closedir(dir);
                    }
                });

                top_list->addItem(new tsl::elm::CategoryHeader("emulation status"));
                top_list->addItem(toggle_item);
                top_list->addItem(game_header);
                //top_list->addItem(new tsl::elm::CategoryHeader("amiibo selection"));
                top_list->addItem(amiibo_header);
                category_header->setValue(std::to_string(count), true);
                top_list->addItem(category_header);
            }
            else {
                top_list->addItem(new tsl::elm::BigCategoryHeader(MakeStatusText(), true));
            }

            root_frame->setClickListener([&](u64 keys) { 
                if(keys & KEY_RSTICK) {
                    if(IsActiveAmiiboValid()) {
                        // User selected the active amiibo, so let's change connection then
                        auto status = emu::GetActiveVirtualAmiiboStatus();
                        switch(status) {
                            case emu::VirtualAmiiboStatus::Connected: {
                                emu::SetActiveVirtualAmiiboStatus(emu::VirtualAmiiboStatus::Disconnected);
                                break;
                            }
                            case emu::VirtualAmiiboStatus::Disconnected: {
                                emu::SetActiveVirtualAmiiboStatus(emu::VirtualAmiiboStatus::Connected);
                                break;
                            }
                            default:
                                break;
                        }
                        Refresh();
                        return true;
                    }
                }
                if(keys & KEY_R) {
                    emu::SetEmulationStatus(emu::EmulationStatus::On);
                    return true;
                }
                if(keys & KEY_L) {
                    emu::SetEmulationStatus(emu::EmulationStatus::Off);
                    return true;
                }
                return false;
            });

            root_frame->setTopSection(top_list);
            root_frame->setBottomSection(bottom_list);
            return this->root_frame;
        }

        virtual void update() override {
            UpdateCurrentApplicationIntercepted();
            this->Refresh();
        }

};

class Overlay : public tsl::Overlay {

    public:
        virtual void initServices() override {
            tsl::hlp::doWithSmSession([&] {
                if(emu::IsAvailable()) {
                    g_emuiibo_init_ok = R_SUCCEEDED(emu::Initialize());
                    if(g_emuiibo_init_ok) {
                        g_emuiibo_version = emu::GetVersion();
                        emu::GetVirtualAmiiboDirectory(g_emuiibo_amiibo_dir, FS_MAX_PATH);
                    }
                }
            });
            if(g_emuiibo_init_ok) {
                UpdateActiveAmiibo();
            }
        }
        
        virtual void exitServices() override {
            emu::Exit();
        }
        
        virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
            return initially<MainGui>();
        }

};

int main(int argc, char **argv) {
    return tsl::loop<Overlay>(argc, argv);
}