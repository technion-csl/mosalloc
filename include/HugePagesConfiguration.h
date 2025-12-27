#ifndef _HUGE_PAGES_CONFIGURATION_H
#define _HUGE_PAGES_CONFIGURATION_H

#include <cstddef>
#include <iostream>
#include "MemoryIntervalList.h"
#include "ParseCsv.h"
#include "MemoryIntervalsValidator.h"

using namespace std;

//TODO: Refactor to one common base class with multipe derived classes
class HugePagesConfiguration {
public:
    enum class ConfigType {
        MMAP_POOL,
        BRK_POOL,
        FILE_BACKED_POOL,
        GENERAL
    };

    struct HugePagesConfigurationParams {
        char* configuration_file;
        size_t _ffa_list_size;
    };

    struct GeneralParams {
        bool _analyze_hpbrs;
        unsigned long _verbose_level;
    };

    HugePagesConfiguration();

    ~HugePagesConfiguration() {}

    HugePagesConfigurationParams &ReadFromEnvironmentVariables(ConfigType type);
    GeneralParams &GetGeneralParams();

private:
    void ReadBrkPoolEnvParams(HugePagesConfigurationParams &params);

    void ReadMmapPoolEnvParams(HugePagesConfigurationParams &params);

    void ReadFileBackedPoolEnvParams(HugePagesConfigurationParams &params);

    void ReadGeneralEnvParams(GeneralParams &params);

    char* GetEnvironmentVariable(const char *key) const;
    unsigned long GetEnvironmentVariableValue(const char *key) const;

    HugePagesConfigurationParams _mmap_pool_params;
    HugePagesConfigurationParams _brk_pool_params;
    HugePagesConfigurationParams _file_backed_pool_params;
    GeneralParams _general_params;

    const size_t KB = 1024;
    const size_t MB = 1024*KB;
    const size_t GB = 1024*MB;

    const char* MMAP_FFA_SIZE_ENV_VAR = "HPC_MMAP_FIRST_FIT_LIST_SIZE";
    const char* FILE_BACKED_FFA_SIZE_ENV_VAR =
          "HPC_FILE_BACKED_FIRST_FIT_LIST_SIZE";
    const char* CONFIGURATION_FILE_ENV_VAR= "HPC_CONFIGURATION_FILE";
    const char* VERBOSE_LEVEL_ENV_VAR = "HPC_VERBOSE_LEVEL";
    const char* DEBUG_BREAK_ENV_VAR = "HPC_DEBUG_BREAK";
    const char* ANALYZE_HPBRS_ENV_VAR = "HPC_ANALYZE_HPBRS";
};

#endif //_HUGE_PAGES_CONFIGURATION_H

