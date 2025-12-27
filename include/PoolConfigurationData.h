//
// Created by yarons-pc on 24/12/2019.
//

#ifndef MOSALLOC_POOLCONFIGURATIONDATA_H
#define MOSALLOC_POOLCONFIGURATIONDATA_H
#include "MemoryIntervalList.h"
class PoolConfigurationData {
public:
    MemoryIntervalList intervalList;
    size_t size;
    PoolConfigurationData();
    ~PoolConfigurationData() {}
};
#endif //MOSALLOC_POOLCONFIGURATIONDATA_H
