#include <sys/mman.h>
#include "ParseCsv.h"
#include "globals.h"
#include <cstdio>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "GlibcAllocationFunctions.h"

#define MOVE_TO_NEXT_LINE()\
    for (; i < size && file_mmap[i++] != '\n'; );

#define NEXT_TOKEN() \
    for (j=0; j<token_size && i<size; i++) { \
        if(file_mmap[i] == ',' || file_mmap[i] == '\n'){ \
            token[j] = 0; \
            if(file_mmap[i] == ',') i++; \
            break; \
        } \
        if (file_mmap[i] == ' '){ \
            continue; \
        } \
        token[j++] = file_mmap[i]; \
    }

int parseCsv::GetConfigFileMaxWindows(const char* path){
    int count = 0;
    int fd;
    struct stat s;
    char *file_mmap;

    // Open the file for reading.
    fd = open (path, O_RDONLY);
    if (fd < 0) {
        THROW_EXCEPTION("can not open csv file");
    }
    
    // Get the size of the file.
    if (fstat (fd, &s) < 0) {
        THROW_EXCEPTION("can not stat the csv file");
    }
    size_t size = s.st_size;

    GlibcAllocationFunctions glibc_funcs;
    // Memory-map the file.
    file_mmap = (char*)glibc_funcs.CallGlibcMmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (file_mmap == MAP_FAILED)
        THROW_EXCEPTION("can not mmap csv file");

    /* count the end-of-line characters */
    for (size_t i = 0; i < size; i++) {
        if (file_mmap[i] == '\n') {
            count++;
        }
    }

    close(fd);
    return count;
}

/**
 *
 * @param ls
 * @param path to csv file in structure:_page_size,_start_offset,_end_offset
 * assuming line lenght at most 1024 chars
 *  assuming the configuration of the file
 *
 */
void parseCsv::ParseCsv(PoolConfigurationData& configurationData, const char* path, const char* poolType){
    int fd;
    struct stat s;
    char *file_mmap;
    size_t token_size = 1024;
    char token[1024] = {0};
    int one_time_size = 0;
    long long int _start_offset, _end_offset, _page_size;

    // Open the file for reading.
    fd = open (path, O_RDONLY);
    if (fd < 0) {
        THROW_EXCEPTION("can not open csv file");
    }
    
    // Get the size of the file.
    if (fstat (fd, &s) < 0) {
        THROW_EXCEPTION("can not stat the csv file");
    }
    size_t size = s.st_size;

    GlibcAllocationFunctions glibc_funcs;
    // Memory-map the file.
    file_mmap = (char*)glibc_funcs.CallGlibcMmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (file_mmap == MAP_FAILED)
        THROW_EXCEPTION("can not mmap csv file");

    size_t i=0, j=0;
    // read the header line
    MOVE_TO_NEXT_LINE()
    // Parse the file
    for (; i < size; i++) {
        NEXT_TOKEN()
        if (token[0] == 0)
            continue;
        if(strcmp(token, poolType)){
            continue;
        }

        NEXT_TOKEN()
        _page_size = atoll(token);
        if( _page_size!=-1 && _page_size!= static_cast<size_t>(PageSize::HUGE_1GB) && _page_size!= static_cast<size_t>(PageSize::HUGE_2MB)){
            THROW_EXCEPTION("unknown page size");
        }

        if(_page_size == -1 ){
            if(!one_time_size){
                one_time_size = 1;
            }
            else THROW_EXCEPTION("pool size already exist");
        }

        NEXT_TOKEN()
        _start_offset = atoll(token);
        if(_start_offset < 0 )
            THROW_EXCEPTION("start offset negative");
        
        NEXT_TOKEN()
        _end_offset = atoll(token);
        if(_end_offset < 0)
            THROW_EXCEPTION("end offset negative");
      
        if(file_mmap[i] != '\n')
            THROW_EXCEPTION("csv configuration file is corrupted!");
        
        if(_page_size !=-1) configurationData.intervalList.AddInterval(_start_offset, _end_offset, (PageSize)_page_size);
        else configurationData.size= _end_offset - _start_offset;
    }
    configurationData.intervalList.Sort();
    close(fd);
}
