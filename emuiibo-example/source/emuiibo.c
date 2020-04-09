#include "emuiibo.h"

static Service g_emuiibo_srv;

#define EMUIIBO_SRV "nfp:emu"

// Note for developers: the IPC command functions which don't return the Result is because emuiibo will always succeed in those specific commands.

bool emuiiboIsAvailable() {
    SmServiceName srv_name = smEncodeName(EMUIIBO_SRV);
    Handle tmph = 0;
    Result rc = smRegisterService(&tmph, srv_name, false, 1);
    if(R_FAILED(rc)) {
        return true;
    }
    smUnregisterService(srv_name);
    return false;
}

Result emuiiboInitialize() {
    if(serviceIsActive(&g_emuiibo_srv)) {
        return 0;
    }
    return smGetService(&g_emuiibo_srv, EMUIIBO_SRV);
}

void emuiiboExit() {
    if(serviceIsActive(&g_emuiibo_srv)) {
        serviceClose(&g_emuiibo_srv);
    }
}

EmuiiboEmulationStatus emuiiboGetEmulationStatus() {
    u32 out = 0;
    serviceDispatchOut(&g_emuiibo_srv, 0, out);
    return (EmuiiboEmulationStatus)out;
}

void emuiiboSetEmulationStatus(EmuiiboEmulationStatus status) {
    u32 in = (u32)status;
    serviceDispatchIn(&g_emuiibo_srv, 1, in);
}

Result emuiiboGetActiveVirtualAmiibo(EmuiiboVirtualAmiiboId *out_amiibo_id, EmuiiboVirtualAmiiboData *out_amiibo_data) {
    return serviceDispatchOut(&g_emuiibo_srv, 2, *out_amiibo_id,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out_amiibo_data, sizeof(EmuiiboVirtualAmiiboData) } },
    );
}

Result emuiiboSetActiveVirtualAmiibo(EmuiiboVirtualAmiiboId *amiibo_id) {
    return serviceDispatchIn(&g_emuiibo_srv, 3, *amiibo_id);
}

void emuiiboResetActiveVirtualAmiibo() {
    serviceDispatch(&g_emuiibo_srv, 4);
}

EmuiiboVirtualAmiiboStatus emuiiboGetActiveVirtualAmiiboStatus() {
    u32 out = 0;
    serviceDispatchOut(&g_emuiibo_srv, 5, out);
    return (EmuiiboVirtualAmiiboStatus)out;
}

void emuiiboSetActiveVirtualAmiiboStatus(EmuiiboVirtualAmiiboStatus status) {
    u32 in = (u32)status;
    serviceDispatchIn(&g_emuiibo_srv, 6, in);
}

Result emuiiboReadNextAvailableVirtualAmiibo(EmuiiboVirtualAmiiboId *out_amiibo_id, EmuiiboVirtualAmiiboData *out_amiibo_data) {
    return serviceDispatchOut(&g_emuiibo_srv, 7, *out_amiibo_id,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out_amiibo_data, sizeof(EmuiiboVirtualAmiiboData) } },
    );
}

void emuiiboResetAvailableVirtualAmiiboIterator() {
    serviceDispatch(&g_emuiibo_srv, 8);
}

EmuiiboVersion emuiiboGetVersion() {
    EmuiiboVersion ver;
    memset(&ver, 0, sizeof(ver));
    serviceDispatchOut(&g_emuiibo_srv, 9, ver);
    return ver;
}