#include <sys/sys_System.hpp>
#include <ipc/mii/mii_Utils.hpp>

#include <ipc/nfp/sys/sys_ISystemManager.hpp>
#include <ipc/nfp/user/user_IUserManager.hpp>
#include <ipc/emu/emu_IEmulationService.hpp>

#define INNER_HEAP_SIZE 0x20000

extern "C" {

    extern u32 __start__;
    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;
    u32 __nx_fsdev_direntry_cache_size = 1;

    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];
    void __libnx_init_time(void);

    void __libnx_initheap(void) {
        void *addr = nx_inner_heap;
        size_t size = nx_inner_heap_size;
        extern char *fake_heap_start;
        extern char *fake_heap_end;
        fake_heap_start = (char*)addr;
        fake_heap_end = (char*)addr + size;
    }

    void __appInit(void) {
        EMU_R_ASSERT(smInitialize());
        EMU_R_ASSERT(fsInitialize());
        EMU_R_ASSERT(fsdevMountSdmc());
        EMU_R_ASSERT(timeInitialize());
        __libnx_init_time();
        EMU_R_ASSERT(hidInitialize());
        EMU_R_ASSERT(ipc::mii::Initialize());
        EMU_R_ASSERT(pmdmntInitialize());
        EMU_R_ASSERT(pminfoInitialize());
        ams::hos::SetVersionForLibnx();
    }

    void __appExit(void) {
        pminfoExit();
        pmdmntExit();
        ipc::mii::Finalize();
        timeExit();
        hidExit();
        fsdevUnmountAll();
        fsExit();
        smExit();
    }

}

namespace ams {

    ncm::ProgramId CurrentProgramId = { 0x0100000000000352ul };

    namespace result {

        bool CallFatalOnResultAssertion = true;

    }

}

namespace {

    struct ServerOptions {
        static const size_t PointerBufferSize = 0x1000;
        static const size_t MaxDomains = 9;
        static const size_t MaxDomainObjects = 10;
    };

    constexpr size_t MaxServers = 4;
    constexpr size_t MaxSessions = 40;

    ams::sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> emuiibo_manager;

}

int main() {
    // Clear previous logs, to avoid extremely big log files
    logger::ClearLogs();
    
    EMU_LOG_FMT("Starting emuiibo...")

    ipc::mii::DumpSystemMiis();
    sys::ScanAmiiboDirectory();
 
    // Register nfp:user
    EMU_R_ASSERT(emuiibo_manager.RegisterMitmServer<ipc::nfp::user::IUserManager>(ipc::nfp::user::ServiceName));

    // Register nfp:sys - why is this still broken?
    // EMU_R_ASSERT(emuiibo_manager.RegisterMitmServer<ipc::nfp::sys::ISystemManager>(ipc::nfp::sys::ServiceName));
    
    // Register custom nfp:emu service
    EMU_R_ASSERT(emuiibo_manager.RegisterServer<ipc::emu::IEmulationService>(ipc::emu::ServiceName, MaxSessions));
 
    emuiibo_manager.LoopProcess();
 
    return 0;
}