
#define TESLA_INIT_IMPL
#include <emuiibo.hpp>
#include <tesla.hpp>
#include <tesla_extensions.hpp>
#include <dirent.h>
#include <filesystem>
#include <upng.h>

namespace {

    bool g_emuiibo_init_ok = false;
    bool g_category_list_update_flag = false;
    int g_category_list_last_index = 0;
    bool g_current_app_intercepted = false;
    char g_emuiibo_amiibo_dir[FS_MAX_PATH];
    emu::Version g_emuiibo_version;

    char g_active_amiibo_path[FS_MAX_PATH];
    emu::VirtualAmiiboData g_active_amiibo_data;

    unsigned char *g_img_buffer = {0};
    int g_img_width;
    bool g_current_img_ok = false;
    std::string g_img_error_msg;
    upng_t* upng;

    inline bool IsActiveAmiiboValid() {
        return strlen(g_active_amiibo_path) > 0;
    }

    inline void UpdateActiveAmiibo() {
        emu::GetActiveVirtualAmiibo(&g_active_amiibo_data, g_active_amiibo_path, FS_MAX_PATH);
    }

    // Returns true if the value changed
    inline bool UpdateCurrentApplicationIntercepted() {
        bool ret = emu::IsCurrentApplicationIdIntercepted();
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

    inline void GetAndResizeImage(int new_height) {
        g_current_img_ok = false;
        g_img_error_msg = "";
        if(IsActiveAmiiboValid()) {
            tsl::hlp::doWithSDCardHandle([new_height] {
                std::string amiibo_png_path = g_active_amiibo_path;
                amiibo_png_path += "/amiibo.png";
                upng = upng_new_from_file(amiibo_png_path.c_str());
                if (upng != NULL) {
                    upng_decode(upng);
                    switch(upng_get_error(upng)) {
                        case UPNG_EOK: {
                            bool is_rgb = ( upng_get_format(upng) == UPNG_RGB8 || upng_get_format(upng) == UPNG_RGB16 );
                            
                            /* 
                            *  DELETE ONCE RGB DOWNSCALE WORKS PROPERLY
                            */ 
                            if (is_rgb){
                                g_img_error_msg = "Please use RGBA PNG.";
                                break;   
                            }
                            /* DELETE END */ 

                            int old_width = upng_get_width(upng);
                            int old_height = upng_get_height(upng);
                            int bpp = upng_get_bpp(upng);
                            int bitdepth = upng_get_bitdepth(upng);
                            int img_depth = bpp/bitdepth;
                            double scale = (double)new_height / (double)old_height;
                            int new_width = old_width*scale;
                            delete[] g_img_buffer;
                            g_img_buffer = new unsigned char [new_width * new_height * img_depth];
                            g_img_width = new_width;

                            
                            /* DEBUG STRING START * /
                            if ( is_rgb ) {
                                g_img_error_msg += "RGB - ";
                                //MAKE SOME MAGIC HERE TO PROPERLY DOWNSCALE RGB
                            } else {
                                g_img_error_msg += "RGBA - ";
                            }
                            / * DEBUG STRING END */

                            g_img_error_msg += std::to_string(bpp) + "/" + std::to_string(bitdepth) + " ";
                            g_img_error_msg += std::to_string(img_depth);
                            
                            for(int h = 0; h != new_height; ++h) {
                                for(int w = 0; w != new_width; ++w) {
                                    int pixel = (h * (new_width *img_depth)) + (w*img_depth);
                                    int nearestMatch =  (((int)(h / scale) * (old_width *img_depth)) + ((int)(w / scale) *img_depth));
                                    for(int d = 0; d != img_depth; ++d) {
                                        g_img_buffer[pixel + d] =  upng_get_buffer(upng)[nearestMatch + d];
                                    }
                                }
                            }
                            g_current_img_ok = true;
                            break;
                        }
                        case UPNG_ENOMEM: {
                            g_img_error_msg = "Image is too big.";
                            break;
                        }
                        case UPNG_ENOTFOUND: {
                            g_img_error_msg = "Image not found.";
                            break;
                        }
                        case UPNG_ENOTPNG: {
                            g_img_error_msg = "Image is not a PNG.";
                            break;
                        }
                        case UPNG_EMALFORMED: {
                            g_img_error_msg = "PNG malformed.";
                            break;
                        }
                        case UPNG_EUNSUPPORTED: {
                            g_img_error_msg = "This PNG not supported.";
                            break;
                        }
                        case UPNG_EUNINTERLACED: {
                            g_img_error_msg = "Image interlacing is not supported.";
                            break;
                        }
                        case UPNG_EUNFORMAT: {
                            g_img_error_msg = "Image color format is not supported.";
                            break;
                        }
                        case UPNG_EPARAM: {
                            g_img_error_msg = "Invalid parameter.";
                            break;
                        }
                    }
                    upng_free(upng);
                }
            });
        }
    }


}

class AmiiboList : public tsl::Gui {

    private:
        tslext::elm::DoubleSectionOverlayFrame *root_frame;
        tslext::elm::CustomCategoryHeader *category_header;
        tslext::elm::SmallToggleListItem *toggle_item = new tslext::elm::SmallToggleListItem("emulation status",emu::GetEmulationStatus()==emu::EmulationStatus::On?true:false,"on","off");
        tslext::elm::SmallListItem *game_header = new tslext::elm::SmallListItem("current game is");
        tslext::elm::SmallListItem *amiibo_header;
        tsl::elm::List *top_list;
        tsl::elm::List *bottom_list;
        std::string amiibo_path;

    public:
        AmiiboList(const std::string &path) : root_frame(new tslext::elm::DoubleSectionOverlayFrame(MakeTitleText(), " ", tslext::SectionsLayout::same, true)), amiibo_path(path) {}

        bool OnItemClick(u64 keys, const std::string &path) {
            if(keys & KEY_A) {
                char amiibo_path[FS_MAX_PATH] = {0};
                strcpy(amiibo_path, path.c_str());
                // Set active amiibo and update our active amiibo value
                emu::SetActiveVirtualAmiibo(amiibo_path, FS_MAX_PATH);
                UpdateActiveAmiibo();
                GetAndResizeImage(80);
                return true;   
            }
            return false;
        }

        virtual tsl::elm::Element *createUI() override {
            top_list = new tsl::elm::List();
            bottom_list = new tsl::elm::List();
            amiibo_header = new tslext::elm::SmallListItem(MakeActiveAmiiboText());
            amiibo_header->setColoredValue(MakeActiveAmiiboStatusText(), emu::GetActiveVirtualAmiiboStatus()==emu::VirtualAmiiboStatus::Disconnected ? tslext::style::color::ColorWarning : tsl::style::color::ColorHighlight);
            
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
                            auto item = new tslext::elm::SmallListItem(data.name);
                            item->setClickListener(std::bind(&AmiiboList::OnItemClick, this, std::placeholders::_1, str_path));
                            bottom_list->addItem(item);
                            count++;
                        }
                    }
                    closedir(dir);
                }
            });

            toggle_item->setClickListener([&](u64 keys) {
                if(keys & KEY_A){
                    if (emu::GetEmulationStatus()==emu::EmulationStatus::On) {
                        emu::SetEmulationStatus(emu::EmulationStatus::Off);
                    } else {
                        emu::SetEmulationStatus(emu::EmulationStatus::On);
                    }
                    return true;
                }
                return false;
            });

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

            top_list->addItem(toggle_item);
            top_list->addItem(game_header);
            top_list->addItem(new tslext::elm::CustomCategoryHeader("current amiibo",false,true));
            top_list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                if(g_current_img_ok){
                    renderer->drawBitmap(x + 5, y + 5, g_img_width, 80, g_img_buffer);
                } else {
                    renderer->drawString(g_img_error_msg.c_str(), false, x + 15, y + 50, 15, renderer->a(tsl::style::color::ColorText));
                }
            }), 90);
            top_list->addItem(amiibo_header);
            category_header = new tslext::elm::CustomCategoryHeader("available amiibos in " + MakeCategoryText(amiibo_path) + ": " + std::to_string(count), false, true); 
            top_list->addItem(category_header);

            root_frame->setTopSection(top_list);
            root_frame->setBottomSection(bottom_list);
            return root_frame;
        }

        virtual void update() override {
            UpdateCurrentApplicationIntercepted();
            this->game_header->setColoredValue(MakeGameInterceptedText(), g_current_app_intercepted ? tsl::style::color::ColorHighlight : tslext::style::color::ColorWarning);
            this->amiibo_header->setText(MakeActiveAmiiboText());
            this->amiibo_header->setColoredValue(MakeActiveAmiiboStatusText(), emu::GetActiveVirtualAmiiboStatus()==emu::VirtualAmiiboStatus::Disconnected ? tslext::style::color::ColorWarning : tsl::style::color::ColorHighlight);
            this->toggle_item->setState(emu::GetEmulationStatus()==emu::EmulationStatus::On?true:false);
        }

};

class MainGui : public tsl::Gui {

    private:
        tslext::elm::SmallToggleListItem *toggle_item = new tslext::elm::SmallToggleListItem("emulation status",emu::GetEmulationStatus()==emu::EmulationStatus::On?true:false,"on","off");
        tslext::elm::SmallListItem *game_header = new tslext::elm::SmallListItem("current game is");
        tsl::elm::List *bottom_list;
        tslext::elm::SmallListItem *amiibo_header;
        tslext::elm::DoubleSectionOverlayFrame *root_frame;
        
    public:
        MainGui() : bottom_list(new tsl::elm::List()), amiibo_header(new tslext::elm::SmallListItem(MakeActiveAmiiboText())), root_frame(new tslext::elm::DoubleSectionOverlayFrame(MakeTitleText(), " ", tslext::SectionsLayout::same, true)) {}

        void Refresh() {
            this->game_header->setColoredValue(MakeGameInterceptedText(), g_current_app_intercepted ? tsl::style::color::ColorHighlight : tslext::style::color::ColorWarning);
            this->amiibo_header->setText(MakeActiveAmiiboText());
            this->amiibo_header->setColoredValue(MakeActiveAmiiboStatusText(), emu::GetActiveVirtualAmiiboStatus()==emu::VirtualAmiiboStatus::Disconnected ? tslext::style::color::ColorWarning : tsl::style::color::ColorHighlight);
            this->toggle_item->setState(emu::GetEmulationStatus()==emu::EmulationStatus::On?true:false);
            if(g_category_list_update_flag) {
                g_category_list_update_flag = false; //back from amiibos
                //if (bottom_list->getItemAtIndex(g_category_list_last_index) != nullptr) //better safe than sorry
                    bottom_list->setFocusedIndex(g_category_list_last_index);
            }
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
                return true;   
            }
            return false;
        }


        virtual tsl::elm::Element *createUI() override {
            auto top_list = new tsl::elm::List();
            
            if(g_emuiibo_init_ok) {
                
                toggle_item->setStateChangedListener([&](bool status) {
                    if(status)
                        emu::SetEmulationStatus(emu::EmulationStatus::Off);
                    else
                        emu::SetEmulationStatus(emu::EmulationStatus::On);
                });
                
                amiibo_header->setClickListener(std::bind(&MainGui::OnAmiiboHeaderClick, this, std::placeholders::_1, g_emuiibo_amiibo_dir));

                // Root
                auto root_item = new tslext::elm::SmallListItem("<root>");
                root_item->setClickListener([&, root_item] (u64 keys) {
                    if(keys & KEY_A) {
                        g_category_list_last_index = bottom_list->getIndexInList(root_item);
                        g_category_list_update_flag = true;
                        tsl::changeTo<AmiiboList>(g_emuiibo_amiibo_dir);
                        return true;
                    }
                    return false;
                });
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
                                auto item = new tslext::elm::SmallListItem(entry->d_name);
                                item->setClickListener([&, item, path] (u64 keys) {
                                    if(keys & KEY_A) {
                                        g_category_list_last_index = bottom_list->getIndexInList(item);
                                        g_category_list_update_flag = true;
                                        tsl::changeTo<AmiiboList>(path);
                                        return true;
                                    }
                                    return false;
                                });
                                bottom_list->addItem(item);
                                count++;
                            }
                        }
                        closedir(dir);
                    }
                });

                top_list->addItem(toggle_item);
                top_list->addItem(game_header);
                top_list->addItem(new tslext::elm::CustomCategoryHeader("current amiibo",false,true));
                top_list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                    if(g_current_img_ok){
                        renderer->drawBitmap(x + 5, y + 5, g_img_width, 80, g_img_buffer);
                    } else {
                        renderer->drawString(g_img_error_msg.c_str(), false, x + 15, y + 50, 15, renderer->a(tsl::style::color::ColorText));
                    }
                }), 90);
                top_list->addItem(amiibo_header);
                top_list->addItem(new tslext::elm::CustomCategoryHeader("available categories: " + std::to_string(count),false,true));
            }
            else {
                top_list->addItem(new tslext::elm::BigCategoryHeader(MakeStatusText(), true));
            }

            toggle_item->setClickListener([&](u64 keys) {
                if(keys & KEY_A){
                    if (emu::GetEmulationStatus()==emu::EmulationStatus::On) {
                        emu::SetEmulationStatus(emu::EmulationStatus::Off);
                    } else {
                        emu::SetEmulationStatus(emu::EmulationStatus::On);
                    }
                    return true;
                }
                return false;
            });

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

class EmuiiboOverlay : public tsl::Overlay {

    public:
        virtual void initServices() override {
            tsl::hlp::doWithSmSession([&] {
                if(emu::IsAvailable()) {
                    g_emuiibo_init_ok = R_SUCCEEDED(emu::Initialize()) && R_SUCCEEDED(pmdmntInitialize()) && R_SUCCEEDED(pminfoInitialize());
                    if(g_emuiibo_init_ok) {
                        g_emuiibo_version = emu::GetVersion();
                        emu::GetVirtualAmiiboDirectory(g_emuiibo_amiibo_dir, FS_MAX_PATH);
                    }
                }
            });
            if(g_emuiibo_init_ok) {
                UpdateActiveAmiibo();
                GetAndResizeImage(80);
            }
        }
        
        virtual void exitServices() override {
            pminfoExit();
            pmdmntExit();
            emu::Exit();
        }
        
        virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
            return initially<MainGui>();
        }

};

int main(int argc, char **argv) {
    return tsl::loop<EmuiiboOverlay>(argc, argv);
}