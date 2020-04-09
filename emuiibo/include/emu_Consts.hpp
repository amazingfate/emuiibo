
#pragma once
#include <string>

namespace consts {

    static inline const std::string EmuDir = "sdmc:/emuiibo";
    static inline const std::string SettingsPath = EmuDir + "/settings.json";
    static inline const std::string LogFilePath = EmuDir + "/emuiibo.log";
    static inline const std::string AmiiboDir = EmuDir + "/amiibo";
    static inline const std::string DumpedMiisDir = EmuDir + "/miis";
    static inline const std::string TempConversionAreasDir = EmuDir + "/temp_areas";

}