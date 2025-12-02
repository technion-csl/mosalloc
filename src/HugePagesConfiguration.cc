#include <iostream>
#include <cstring>
#include "HugePagesConfiguration.h"
#include "globals.h"

HugePagesConfiguration::HugePagesConfiguration() {
    ReadBrkPoolEnvParams(_brk_pool_params);
    ReadMmapPoolEnvParams(_mmap_pool_params);
    ReadFileBackedPoolEnvParams(_file_backed_pool_params);
    ReadGeneralEnvParams(_general_params);
}

HugePagesConfiguration::HugePagesConfigurationParams &
HugePagesConfiguration::ReadFromEnvironmentVariables(
        HugePagesConfiguration::ConfigType type) {
    switch (type) {
        case ConfigType::BRK_POOL:
            return _brk_pool_params;
        case ConfigType::MMAP_POOL:
            return _mmap_pool_params;
        case ConfigType::FILE_BACKED_POOL:
            return _file_backed_pool_params;
        default:
            THROW_EXCEPTION("invalid type!");
    }
}

HugePagesConfiguration::GeneralParams &
HugePagesConfiguration::GetGeneralParams() {
    return _general_params; 
}


//NOTE: how this return value are allocated ? if it static allocated dont we need to give size?
char* HugePagesConfiguration::GetEnvironmentVariable(const char *key) const {
    char *val = getenv(key);
    if (val == NULL) {
        THROW_EXCEPTION("environment key was not found");
    }
    return val;
}

unsigned long HugePagesConfiguration::GetEnvironmentVariableValue(
        const char *key) const {
    char *val = GetEnvironmentVariable(key);
    return stoul(val);
}

//Note: using default value to env var.
void HugePagesConfiguration::ReadGeneralEnvParams(
        HugePagesConfiguration::GeneralParams &params) {
    char *analyze_val = getenv(ANALYZE_HPBRS_ENV_VAR);
    params._analyze_hpbrs = (analyze_val == NULL) ? false 
        : (stoul(analyze_val) != 0);
    
    char *verbose_val = getenv(VERBOSE_LEVEL_ENV_VAR);
    params._verbose_level = (verbose_val == NULL) ? 0 : stoul(verbose_val);
}

void HugePagesConfiguration::ReadMmapPoolEnvParams(
        HugePagesConfiguration::HugePagesConfigurationParams &params) {
    params.configuration_file =
            GetEnvironmentVariable(CONFIGURATION_FILE_ENV_VAR);
    params._ffa_list_size = GetEnvironmentVariableValue(MMAP_FFA_SIZE_ENV_VAR);
}

void HugePagesConfiguration::ReadBrkPoolEnvParams(
        HugePagesConfiguration::HugePagesConfigurationParams &params) {
    params.configuration_file = GetEnvironmentVariable(CONFIGURATION_FILE_ENV_VAR);
    params._ffa_list_size = 0;
}

void HugePagesConfiguration::ReadFileBackedPoolEnvParams(
        HugePagesConfiguration::HugePagesConfigurationParams &params) {
    params.configuration_file = nullptr;
    params._ffa_list_size = GetEnvironmentVariableValue(
            FILE_BACKED_FFA_SIZE_ENV_VAR);
}

