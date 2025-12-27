#ifndef _NUMA_MAPS_H
#define _NUMA_MAPS_H

#include <sys/types.h>
#include <vector>
#include "../include/globals.h"

class NumaMaps {
public:
    /**********************************************
    * Public inner classes
    **********************************************/
    struct MemoryRange {
        enum class Type {
            ANONYMOUS,
            HEAP,
            STACK,
            FILE_MAPPED,
            OTHER
        };

        MemoryRange(void *start_address,
                    NumaMaps::MemoryRange::Type type,
                    unsigned long total_pages,
                    unsigned long dirty_pages,
                    PageSize page_size,
                    size_t total_size,
                    std::vector<unsigned long> pages_in_node);

        ~MemoryRange();

        void *_start_address;
        NumaMaps::MemoryRange::Type _type;
        unsigned long _total_pages;
        unsigned long _dirty_pages;
        PageSize _page_size;
        size_t _total_size;
        std::vector<unsigned long> _pages_in_node;
    };

    /**********************************************
    * Public methods
    **********************************************/
    NumaMaps(pid_t pid);

    ~NumaMaps();

    void Reload();

    unsigned long GetTotalAnonymousPages(PageSize page_size);

    size_t GetTotalAnonymousMemory();

    const MemoryRange &GetMemoryRange(void *start_address);

private:

    /**********************************************
    * Private inner classes
    **********************************************/

    /**********************************************
    * Private methods
    **********************************************/
    void ParseNumaMapsFile();

    unsigned long GetNumaNodesCount();

    /**********************************************
    * Data members
    **********************************************/
    pid_t _pid;
    unsigned long _numa_nodes;
    std::vector<MemoryRange> _numa_maps;
};

#endif //_NUMA_MAPS_H
