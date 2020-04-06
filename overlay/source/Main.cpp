
#define TESLA_INIT_IMPL
#include <emuiibo.hpp>
#include <libtesla_ext.hpp>
#include <sstream>

namespace {

    bool g_emuiibo_init_ok = false;
    emu::VirtualAmiibo g_active_amiibo;
    std::vector<emu::VirtualAmiibo> g_amiibo_list;

    inline void ClearAmiiboList() {
        for(auto &amiibo: g_amiibo_list) {
            amiibo.Close();
        }
        g_amiibo_list.clear();
    }

    inline std::string MakeAvailableAmiibosText() {
        return "Available virtual amiibos (" + std::to_string(g_amiibo_list.size()) + ")";
    }

    inline std::string MakeActiveAmiiboText() {
        if(g_active_amiibo.IsValid()) {
            return g_active_amiibo.GetName();
        }
        return "No active virtual amiibo";
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
            return "Unable to access emuiibo...";
        }
        std::string msg = "Emulation: ";
        auto e_status = emu::GetEmulationStatus();
        switch(e_status) {
            case emu::EmulationStatus::On: {
                msg += "on";
                break;
            }
            case emu::EmulationStatus::Off: {
                msg += "off";
                break;
            }
        }
        return msg;
    }

    inline std::string MakeActiveAmiiboStatusText() {
        std::string msg = "";
        if(!g_active_amiibo.IsValid()) {
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
/*
class AmiibosList : public tsl::Gui {

    private:
        tsl::elm::CustomOverlayFrame *root_frame;
        tsl::elm::BigCategoryHeader *selected_header;
        tsl::elm::CategoryHeader *count_header;
        tsl::elm::List *list;
        tsl::elm::List *header_list;

    public:
        AmiibosList() : root_frame(new tsl::elm::CustomOverlayFrame(MakeTitleText(), MakeStatusText())) {}

        virtual tsl::elm::Element *createUI() override {
            list = new tsl::elm::List();
            header_list = new tsl::elm::List();

            selected_header = new tsl::elm::BigCategoryHeader(MakeActiveAmiiboText(), true);
            count_header = new tsl::elm::CategoryHeader(MakeAvailableAmiibosText(), true);

            header_list->addItem(selected_header);
            header_list->addItem(count_header);
            
            for(auto &amiibo: g_amiibo_list) {
                auto *item = new tsl::elm::SmallListItem(amiibo.GetName());
                item->setClickListener([&](u64 keys) {
                    if(keys & KEY_A) {
                        if(g_active_amiibo.IsValid()) {
                            if(amiibo.GetName() == g_active_amiibo.GetName()) {
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
                        // Set active amiibo and update our active amiibo value
                        amiibo.SetAsActiveVirtualAmiibo();
                        g_active_amiibo.Close();
                        emu::GetActiveVirtualAmiibo(g_active_amiibo);
                        selected_header->setText(MakeActiveAmiiboText());
                        root_frame->setSubtitle(MakeStatusText());
                        return true;   
                    }
                    return false;
                });
                list->addItem(item);
            }

            root_frame->setHeader(header_list);
            root_frame->setContent(list);
            return root_frame;
        }

};
*/
class EmuiiboGui : public tsl::Gui {

    private:
        tsl::elm::SmallListItem *amiibo_header;
        tsl::elm::DoubleSectionOverlayFrame *root_frame;
        tsl::elm::NamedStepTrackBar *toggle_item = new tsl::elm::NamedStepTrackBar("\u22EF", { "Off", "On" });
        
    public:
        EmuiiboGui() : amiibo_header(new tsl::elm::SmallListItem(MakeActiveAmiiboText())), root_frame(new tsl::elm::DoubleSectionOverlayFrame(MakeTitleText(), MakeAvailableAmiibosText(), tsl::SectionsLayout::big_bottom, true)) {}

        virtual tsl::elm::Element *createUI() override {
            auto top_list = new tsl::elm::List();
            auto bottom_list = new tsl::elm::List();
            
            if(g_emuiibo_init_ok) {
                //auto status = emu::GetEmulationStatus();

                //auto *toggle_item = new tsl::elm::NamedStepTrackBar("\u22EF", { "Off", "On" });
                


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
                        if(g_active_amiibo.IsValid()) {
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
                top_list->addItem(amiibo_header);

                for(auto &amiibo: g_amiibo_list) {
                    auto *item = new tsl::elm::SmallListItem(amiibo.GetName());
                    item->setClickListener([&](u64 keys) {
                        if(keys & KEY_A) {
                            /* Not sure if cconnect/disconnect should happen here too. Not needed imho
                            if(g_active_amiibo.IsValid()) {
                                if(amiibo.GetName() == g_active_amiibo.GetName()) {
                                    // User selected the active amiibo, so let's change connection then
                                    auto status = emu::GetActiveVirtualAmiiboStatus();
                                    switch(status) {
                                        case emu::VirtualAmiiboStatus::Connected: {
                                            emu::SetActiveVirtualAmiiboStatus(emu::VirtualAmiiboStatus::Disconnected);
                                            //root_frame->setSubtitle(MakeStatusText());
                                            break;
                                        }
                                        case emu::VirtualAmiiboStatus::Disconnected: {
                                            emu::SetActiveVirtualAmiiboStatus(emu::VirtualAmiiboStatus::Connected);
                                            //root_frame->setSubtitle(MakeStatusText());
                                            break;
                                        }
                                        default:
                                            break;
                                    }
                                    return true;
                                }
                            }
                            */
                            // Set active amiibo and update our active amiibo value
                            amiibo.SetAsActiveVirtualAmiibo();
                            g_active_amiibo.Close();
                            emu::GetActiveVirtualAmiibo(g_active_amiibo);
                            return true;   
                        }
                        return false;
                    });
                    bottom_list->addItem(item);
                }

            }
            else {
                top_list->addItem(new tsl::elm::BigCategoryHeader(MakeStatusText(), true));
            }

            root_frame->setClickListener([&](u64 keys) { 
                if(keys & KEY_RSTICK) {
                    if(g_active_amiibo.IsValid()) {
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
            amiibo_header->setText(MakeActiveAmiiboText());
            amiibo_header->setValue(MakeActiveAmiiboStatusText());
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
                emu::GetActiveVirtualAmiibo(g_active_amiibo);
                g_amiibo_list = emu::ListAmiibos();
            }
        }
        
        virtual void exitServices() override {
            ClearAmiiboList();
            emu::Exit();
        }
        
        virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
            return initially<EmuiiboGui>();
        }

};

int main(int argc, char **argv) {
    return tsl::loop<Overlay>(argc, argv);
}