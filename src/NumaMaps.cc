#include <stdexcept>
#include <regex>
#include <fstream>
#include <iostream>
#include <dirent.h>

#include "NumaMaps.h"

NumaMaps::MemoryRange::MemoryRange(void *start_address,
                                   NumaMaps::MemoryRange::Type type,
                                   unsigned long total_pages,
                                   unsigned long dirty_pages,
                                   PageSize page_size,
                                   size_t total_size,
                                   std::vector<unsigned long> pages_in_node) :
        _start_address(start_address),
        _type(type),
        _total_pages(total_pages),
        _dirty_pages(dirty_pages),
        _page_size(page_size),
        _total_size(total_size),
        _pages_in_node(pages_in_node) {}

NumaMaps::MemoryRange::~MemoryRange() {}

PageSize CastPageSizeString(std::string page_size_in_kb_str) {
    size_t page_size_in_kb = stoul(page_size_in_kb_str);
    if (page_size_in_kb == 4) {
        return PageSize::BASE_4KB;
    } else if (page_size_in_kb == 2048) {
        return PageSize::HUGE_2MB;
    } else if (page_size_in_kb == 1048576) {
        return PageSize::HUGE_1GB;
    } else {
        throw std::invalid_argument("unsupported page size!");
    }
}

NumaMaps::NumaMaps(pid_t pid) : _pid(pid) {
    _numa_nodes = GetNumaNodesCount();
    ParseNumaMapsFile();
}

NumaMaps::~NumaMaps() {
    _numa_maps.clear();
}

/*
 * Parse and store current numa-maps for further queries
 */
void NumaMaps::ParseNumaMapsFile() {
    // Parse process memory mappings according to /proc/<pid>/numa_maps
    // for more details: man numa
    std::string file_name = "/proc/" + std::to_string(_pid) + "/numa_maps";
    std::ifstream numa_maps_f(file_name);
    std::smatch m;
    std::string line;

    while (std::getline(numa_maps_f, line)) {
        // Parse memory region start address
        void *start_address = nullptr;
        if (std::regex_search(line, m,
                              std::regex("(0[xX])?[0-9a-fA-F]+"))) {
            start_address = (void *) std::stoul(m[0], nullptr, 16);
        }

        // Parse page size
        PageSize page_size = PageSize::UNKNOWN;
        if (std::regex_search(line, m,
                              std::regex("kernelpagesize_kB=(\\d*)"))) {
            page_size = CastPageSizeString(m[1]);
        }

        // Prase dirty pages
        unsigned long dirty_pages = 0;
        if (std::regex_search(line, m, std::regex("dirty=(\\d*)"))) {
            dirty_pages = stoul(m[1]);
        }

        NumaMaps::MemoryRange::Type type = MemoryRange::Type::OTHER;
        // Prase type (anonymous / heap / stack / file-mapped)
        if (std::regex_search(line, m, std::regex("anon=(\\d*)"))) {
            // m[1] will hold the anon pages
            type = NumaMaps::MemoryRange::Type::ANONYMOUS;
        }
        if (std::regex_search(line, m, std::regex("stack"))) {
            type = NumaMaps::MemoryRange::Type::STACK;
        }
        if (std::regex_search(line, m, std::regex("heap"))) {
            type = NumaMaps::MemoryRange::Type::HEAP;
        }
        if (std::regex_search(line, m, std::regex("mapped="))) {
            type = NumaMaps::MemoryRange::Type::FILE_MAPPED;
        }

        std::vector<unsigned long> pages_in_node(_numa_nodes);
        unsigned long total_pages = 0;
        // Parse Numa Nodes allocation
        for (unsigned long node = 0; node < _numa_nodes; node++) {
            std::string r = "N" + std::to_string(node) + "=(\\d*)";
            if (std::regex_search(line, m, std::regex(r))) {
                pages_in_node[node] = stoul(m[1]);
                total_pages += pages_in_node[node];
            }
        }
        unsigned long total_size = total_pages * static_cast<size_t>(page_size);

        MemoryRange memory_range(start_address, type, total_pages,
                                 dirty_pages, page_size, total_size,
                                 pages_in_node);

        _numa_maps.push_back(memory_range);
    }
}

/*
 * Re-parse current numa-maps
 */
void NumaMaps::Reload() {
    _numa_maps.clear();
    ParseNumaMapsFile();
}

/*
 * Return total anonymous pages (for the given @page_size) allocated for
 * current numa-maps
 */
unsigned long NumaMaps::GetTotalAnonymousPages(PageSize page_size) {
    unsigned long total_pages = 0;
    for (auto mem_range : _numa_maps) {
        if ((mem_range._type == NumaMaps::MemoryRange::Type::ANONYMOUS) &&
            (mem_range._page_size == page_size)) {
            for (auto pages_in_node : mem_range._pages_in_node) {
                total_pages += pages_in_node;
            }
        }
    }
    return total_pages;
}

/*
 * Return total anonymous pages (for all pages sizes) allocated for current
 * numa-maps
 */
size_t NumaMaps::GetTotalAnonymousMemory() {
    return GetTotalAnonymousPages(PageSize::BASE_4KB) *
                   static_cast<size_t>(PageSize::BASE_4KB) +
           GetTotalAnonymousPages(PageSize::HUGE_2MB) *
                   static_cast<size_t>(PageSize::HUGE_2MB) +
           GetTotalAnonymousPages(PageSize::HUGE_1GB) *
                   static_cast<size_t>(PageSize::HUGE_1GB);
}

/*
 * Get the number of Numa Nodes in current system by reading
 * /sys/devices/system/node file system and count nodeX folders inside it.
 */
unsigned long NumaMaps::GetNumaNodesCount() {
    DIR *dir;
    struct dirent *ent;
    unsigned long count = 0;
    if ((dir = opendir("/sys/devices/system/node")) == NULL) {
        std::error_code ec(errno, std::generic_category());
        throw std::system_error(ec,
                                "could not open /sys/devices/system/node dir!");
    }
    // Find all nodeX folders to find out numa nodes count in the system
    std::smatch m;
    while ((ent = readdir(dir)) != NULL) {
        std::string s(ent->d_name);
        if (std::regex_search(s, m,
                              std::regex("node(\\d*)"))) {
            // +1 because nodes indices are zero indexed
            unsigned long node_num = stoul(m[1]) + 1;
            if (node_num > count) {
                count = node_num;
            }
        }
    }
    closedir(dir);
    return count;
}

const NumaMaps::MemoryRange &NumaMaps::GetMemoryRange(void *start_address) {
    for (NumaMaps::MemoryRange &mem_range : _numa_maps) {
        if (mem_range._start_address == start_address) {
            return mem_range;
        }
    }
    throw std::runtime_error("map mem_range address not found.");
}
