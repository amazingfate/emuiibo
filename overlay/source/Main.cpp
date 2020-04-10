
#define TESLA_INIT_IMPL
#include <emuiibo.hpp>
#include <libtesla_ext.hpp>
#include <filesystem>

namespace {

    bool g_emuiibo_init_ok = false;
    bool g_active_amiibo_valid = false;
    bool g_current_app_intercepted = false;
    emu::VirtualAmiiboId g_active_amiibo_id;
    emu::VirtualAmiiboData g_active_amiibo_data;
    u32 g_virtual_amiibo_count = 0;

    inline void UpdateActiveAmiibo() {
        g_active_amiibo_valid = R_SUCCEEDED(emu::GetActiveVirtualAmiibo(&g_active_amiibo_id, &g_active_amiibo_data));
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

    inline std::string MakeAvailableAmiibosText() {
        return "Available virtual amiibos (" + std::to_string(g_virtual_amiibo_count) + ")";
    }

    inline std::string MakeActiveAmiiboDirectoryText() {
        std::string msg = "";
        if(g_active_amiibo_valid) {
            std::filesystem::path path = g_active_amiibo_data.path;
            msg = path.parent_path().filename();
        }
        return msg;
    }

    inline std::string MakeActiveAmiiboText() {
        std::string msg = "No active virtual amiibo";
        if(g_active_amiibo_valid) {
            msg = g_active_amiibo_data.name;
        }
        return msg;
    }

    inline std::string MakeTitleText() {
        if(!g_emuiibo_init_ok) {
            return "emuiibo";
        }
        auto ver = emu::GetVersion();
        return "emuiibo v" + std::to_string(ver.major) + "." + std::to_string(ver.minor) + "." + std::to_string(ver.micro) + " (" + (ver.dev_build ? "dev" : "release") + ")";
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
        if(!g_active_amiibo_valid) {
            return msg;
        } else {
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
    
}

class EmuiiboGui : public tsl::Gui {

    private:
        tsl::elm::CategoryHeader *amiibo_dir_header;
        tsl::elm::SmallListItem *amiibo_header;
        tsl::elm::DoubleSectionOverlayFrame *root_frame;
        tsl::elm::NamedStepTrackBar *toggle_item = new tsl::elm::NamedStepTrackBar("\u22EF", { "Off", "On" });
        tsl::elm::SmallListItem *game_header = new tsl::elm::SmallListItem("Current game is");
        
    public:
        EmuiiboGui() : amiibo_dir_header(new tsl::elm::CategoryHeader(MakeActiveAmiiboDirectoryText())), amiibo_header(new tsl::elm::SmallListItem(MakeActiveAmiiboText())), root_frame(new tsl::elm::DoubleSectionOverlayFrame(MakeTitleText(), MakeAvailableAmiibosText(), tsl::SectionsLayout::same, true)) {}

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

                amiibo_header->setClickListener([&](u64 keys) { 
                    if(keys & KEY_A) {
                        if(g_active_amiibo_valid) {
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
                            return true;
                        }
                    }
                    return false;
                });

                top_list->addItem(new tsl::elm::CategoryHeader("emulation status"));
                top_list->addItem(toggle_item);
                top_list->addItem(game_header);
                top_list->addItem(amiibo_dir_header);
                top_list->addItem(amiibo_header);

                u32 count = 0;
                // Reset the internal iterator, to start from the beginning
                emu::ResetAvailableVirtualAmiiboIterator();
                while(true) {
                    emu::VirtualAmiiboId id = {};
                    emu::VirtualAmiiboData data = {};
                    if(R_FAILED(emu::ReadNextAvailableVirtualAmiibo(&id, &data))) {
                        break;
                    }
                    count++;
                    auto *item = new tsl::elm::SmallListItem(data.name);
                    item->setClickListener([id, this](u64 keys) {
                        if(keys & KEY_A) {
                            // Set active amiibo and update our active amiibo value
                            emu::SetActiveVirtualAmiibo(const_cast<emu::VirtualAmiiboId*>(&id));
                            UpdateActiveAmiibo();

                            return true;   
                        }
                        return false;
                    });
                    bottom_list->addItem(item);
                }
                g_virtual_amiibo_count = count;

            }
            else {
                top_list->addItem(new tsl::elm::BigCategoryHeader(MakeStatusText(), true));
            }

            root_frame->setClickListener([&](u64 keys) { 
                if(keys & KEY_RSTICK) {
                    if(g_active_amiibo_valid) {
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
            return root_frame;
        }

        virtual void update() override {
            root_frame->setSubtitle(MakeAvailableAmiibosText());
            //UpdateCurrentApplicationIntercepted();
            game_header->setColoredValue(MakeGameInterceptedText(), g_current_app_intercepted ? tsl::style::color::ColorHighlight : tsl::style::color::ColorWarning);
            amiibo_dir_header->setText(MakeActiveAmiiboDirectoryText());
            amiibo_header->setText(MakeActiveAmiiboText());
            amiibo_header->setColoredValue(MakeActiveAmiiboStatusText(), emu::GetActiveVirtualAmiiboStatus()==emu::VirtualAmiiboStatus::Disconnected ? tsl::style::color::ColorWarning : tsl::style::color::ColorHighlight);
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
        }
};

class Overlay : public tsl::Overlay {

    public:
        virtual void initServices() override {
            tsl::hlp::doWithSmSession([&] {
                if(emu::IsAvailable()) {
                    g_emuiibo_init_ok = R_SUCCEEDED(emu::Initialize());
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
            return initially<EmuiiboGui>();
        }

};

int main(int argc, char **argv) {
    return tsl::loop<Overlay>(argc, argv);
}