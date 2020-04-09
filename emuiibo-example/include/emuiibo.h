
#pragma once
#include <switch.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
Source for emuiibo's IPC interface to control virtual amiibo emulation
You can write your own IPC code if you know what you're doing, or use this sources (emuiibo.h and emuiibo.c)

NOTE: this sources are made to work with emuiibo 0.5 - emuiibo's IPC system has almost entirely changed from 0.4 to 0.5, so use these ONLY with 0.5!
This code won't work fine with <0.5 emuiibo.
*/

typedef struct {
    u8 id[0x10];
} EmuiiboVirtualAmiiboId;

NX_CONSTEXPR bool emuiiboEqualVirtualAmiiboIds(EmuiiboVirtualAmiiboId *a, EmuiiboVirtualAmiiboId *b) {
    return memcmp(a->id, b->id, 0x10) == 0;
}

typedef struct {
    bool random_uuid;
    u8 uuid[10];
} EmuiiboVirtualAmiiboUuidInfo;

typedef struct {
    u16 year;
    u8 month;
    u8 day;
} EmuiiboVirtualAmiiboDate;

typedef struct {
    EmuiiboVirtualAmiiboUuidInfo uuid;
    char name[40 + 1];
    char path[FS_MAX_PATH];
    EmuiiboVirtualAmiiboDate first_write_date;
    EmuiiboVirtualAmiiboDate last_write_date;
    NfpMiiCharInfo mii_charinfo;
} EmuiiboVirtualAmiiboData;

typedef enum {
    EmuiiboEmulationStatus_On,
    EmuiiboEmulationStatus_Off,
} EmuiiboEmulationStatus;

typedef enum {
    EmuiiboVirtualAmiiboStatus_Invalid,
    EmuiiboVirtualAmiiboStatus_Connected,
    EmuiiboVirtualAmiiboStatus_Disconnected,  
} EmuiiboVirtualAmiiboStatus;

typedef struct {
    u8 major;
    u8 minor;
    u8 micro;
    u8 dev_build;
} EmuiiboVersion;

NX_CONSTEXPR bool emuiiboVersionIsDevBuild(const EmuiiboVersion *ver) {
    return !!ver->dev_build;
}

NX_CONSTEXPR bool emuiiboVersionMatches(const EmuiiboVersion *ver, u8 major, u8 minor, u8 micro) {
    return (ver->major == major) && (ver->minor == minor) && (ver->micro == micro);
}

typedef enum {
    Module_Emuiibo = 352
} EmuiiboResultModule;

typedef enum {
    EmuiiboError_NoAmiiboLoaded = 1,
    EmuiiboError_UnableToMove = 2,
    EmuiiboError_StatusOff = 3
} EmuiiboResultDescription;

// Note: the service's name is "nfp:emu"

bool emuiiboIsAvailable();

Result emuiiboInitialize();
void emuiiboExit();

EmuiiboVersion emuiiboGetVersion();

EmuiiboEmulationStatus emuiiboGetEmulationStatus();
void emuiiboSetEmulationStatus(EmuiiboEmulationStatus status);

Result emuiiboGetActiveVirtualAmiibo(EmuiiboVirtualAmiiboId *out_amiibo_id, EmuiiboVirtualAmiiboData *out_amiibo_data);
Result emuiiboSetActiveVirtualAmiibo(EmuiiboVirtualAmiiboId *amiibo_id);
void emuiiboResetActiveVirtualAmiibo();

EmuiiboVirtualAmiiboStatus emuiiboGetActiveVirtualAmiiboStatus();
void emuiiboSetActiveVirtualAmiiboStatus(EmuiiboVirtualAmiiboStatus status);

Result emuiiboReadNextAvailableVirtualAmiibo(EmuiiboVirtualAmiiboId *out_amiibo_id, EmuiiboVirtualAmiiboData *out_amiibo_data);
void emuiiboResetAvailableVirtualAmiiboIterator();

void emuiiboIsApplicationIdIntercepted(u64 app_id, bool *out_intercepted);
void emuiiboIsCurrentApplicationIdIntercepted(bool *out_intercepted);

#ifdef __cplusplus
}
#endif