#include <sys/sys_Emulation.hpp>
#include <algorithm>

namespace sys {

    static Lock g_emulation_lock(true);
    
    static EmulationStatus g_emulation_status = EmulationStatus::Off;
    static amiibo::VirtualAmiibo g_virtual_amiibo;
    static VirtualAmiiboStatus g_virtual_amiibo_status = VirtualAmiiboStatus::Invalid;
    
    static std::vector<u64> g_intercepted_app_id_list;

    EmulationStatus GetEmulationStatus() {
        EMU_LOCK_SCOPE_WITH(g_emulation_lock);
        return g_emulation_status;
    }

    void SetEmulationStatus(EmulationStatus status) {
        EMU_LOCK_SCOPE_WITH(g_emulation_lock);
        g_emulation_status = status;
    }

    amiibo::VirtualAmiibo &GetActiveVirtualAmiibo() {
        EMU_LOCK_SCOPE_WITH(g_emulation_lock);
        return g_virtual_amiibo;
    }

    bool IsActiveVirtualAmiiboValid() {
        EMU_LOCK_SCOPE_WITH(g_emulation_lock);
        return g_virtual_amiibo.IsValid();
    }

    void SetActiveVirtualAmiibo(amiibo::VirtualAmiibo amiibo) {
        EMU_LOCK_SCOPE_WITH(g_emulation_lock);
        g_virtual_amiibo = amiibo;
        SetActiveVirtualAmiiboStatus(VirtualAmiiboStatus::Connected);
    }

    VirtualAmiiboStatus GetActiveVirtualAmiiboStatus() {
        EMU_LOCK_SCOPE_WITH(g_emulation_lock);
        if(!IsActiveVirtualAmiiboValid()) {
            g_virtual_amiibo_status = VirtualAmiiboStatus::Invalid;
        }
        return g_virtual_amiibo_status;
    }

    void SetActiveVirtualAmiiboStatus(VirtualAmiiboStatus status) {
        EMU_LOCK_SCOPE_WITH(g_emulation_lock);
        EMU_LOG_FMT("Setting new virtual amiibo status: " << static_cast<u32>(status))
        if(IsActiveVirtualAmiiboValid()) {
            g_virtual_amiibo_status = status;
        }
        else {
            g_virtual_amiibo_status = VirtualAmiiboStatus::Invalid;
        }
    }

    Result GetCurrentApplicationId(u64 *out_app_id) {
        u64 tmp_pid = 0;
        R_TRY(pmdmntGetApplicationProcessId(&tmp_pid));
        R_TRY(pminfoGetProgramId(out_app_id, tmp_pid));
        return 0;
    }

    void RegisterInterceptedApplicationId(u64 app_id) {
        EMU_LOCK_SCOPE_WITH(g_emulation_lock);
        if(!IsApplicationIdIntercepted(app_id)) {
            g_intercepted_app_id_list.push_back(app_id);
        }
    }

    void UnregisterInterceptedApplicationId(u64 app_id) {
        EMU_LOCK_SCOPE_WITH(g_emulation_lock);
        if(IsApplicationIdIntercepted(app_id)) {
            g_intercepted_app_id_list.erase(std::remove(g_intercepted_app_id_list.begin(), g_intercepted_app_id_list.end(), app_id), g_intercepted_app_id_list.end());
        }
    }

    bool IsApplicationIdIntercepted(u64 app_id) {
        EMU_LOCK_SCOPE_WITH(g_emulation_lock);
        return std::find(g_intercepted_app_id_list.begin(), g_intercepted_app_id_list.end(), app_id) != g_intercepted_app_id_list.end();
    }

}