//
// Created by yarons-pc on 17/11/2019.
//

#ifndef MOSALLOC_PARSECSV_H
#define MOSALLOC_PARSECSV_H
#include "PoolConfigurationData.h"

#include "globals.h"

class parseCsv {
public:
    parseCsv() {}
    ~parseCsv() {}
    /***
     This function parse csv file in the following format:
         _______________________________________________
        |"type", "pageSize", "startOffset", "endOffset" |
        |"mmap", -1 , 0 , 164326                        |
        |"mmap", 1073741824, 0, 1073741824              |
        |"mmap", 2097152, ?, ?                          |
        |"brk", -1 , 0 , ?                              |
        |"brk", 2097152, ?, ?                           |
        |"brk", 2097152, ?, ?                           |
        |"file", -1 , 0 , ?                             |

     * @param configurationData -- allocated object to put result inside.
     * @param path -- path to configuration file (csv)
     * @param poolType -- the pool type, support {"mmap", "brk", file")
     */
    static void ParseCsv(PoolConfigurationData& configurationData, const char* path, const char* poolType);
    static int GetConfigFileMaxWindows(const char* path);
};

#endif //MOSALLOC_PARSECSV_H
