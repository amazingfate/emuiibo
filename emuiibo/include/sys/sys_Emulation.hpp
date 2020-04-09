
#pragma once
#include <amiibo/amiibo_Formats.hpp>

namespace sys {
    
    enum class EmulationStatus : u32 {
        On,
        Off,
    };

    enum class VirtualAmiiboStatus : u32 {
        Invalid,
        Connected,
        Disconnected
    };

    EmulationStatus GetEmulationStatus();
    void SetEmulationStatus(EmulationStatus status);

    amiibo::VirtualAmiibo &GetActiveVirtualAmiibo();
    bool IsActiveVirtualAmiiboValid();
    void SetActiveVirtualAmiibo(amiibo::VirtualAmiibo amiibo);

    VirtualAmiiboStatus GetActiveVirtualAmiiboStatus();
    void SetActiveVirtualAmiiboStatus(VirtualAmiiboStatus status);

    Result GetCurrentApplicationId(u64 *out_app_id);
    void RegisterInterceptedApplicationId(u64 app_id);
    void UnregisterInterceptedApplicationId(u64 app_id);
    bool IsApplicationIdIntercepted(u64 app_id);

    inline bool IsCurrentApplicationIdIntercepted() {
        u64 cur_app_id = 0;
        if(R_SUCCEEDED(GetCurrentApplicationId(&cur_app_id))) {
            return IsApplicationIdIntercepted(cur_app_id);
        }
        return false;
    }
    
}