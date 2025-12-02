//
// Created by User on 17/11/2019.
//

#ifndef MOSALLOC_MEMORYINTERVALSVALIDATOR_H
#define MOSALLOC_MEMORYINTERVALSVALIDATOR_H


#include <MemoryIntervalList.h>

enum class ValidatorErrorMessage {
    SUCCESS,
    INVALID_1GB_START_OFFSET,
    OFFSET_BETWEEN_TWO_1GB_INTERVALS_ERROR,
    SIZE_OF_1GB_INTERVAL_ERROR,
    INVALID_2MB_START_OFFSET,
    OFFSET_BETWEEN_TWO_2MB_INTERVALS_ERROR,
    SIZE_OF_2MB_INTERVAL_ERROR,
    OFFSET_BETWEEN_1GB_AND_2MB_INTERVALS_ERROR,
    INVALID_PAGE_SIZE
};

class MemoryIntervalsValidator {
public:
    /***
    Validate the following conditions:
    1. Each MemoryInterval page size is 1GB or 2MB
    2. For each interval back by 1GB pages:
          * each interval start from address is multiply of 4KB
          * distance between intervals are multiply of 1GB (1GB_page.end(i) - 1GB_page.start(i+1) % 1GB == 0)
          * size of interval is multiply of 1GB (1GB_page.end(i) - 1GB_page.start(i) % 1GB == 0)
    3. For each interval back by 2MB pages:
        the same three checks
    4. The distance between 1GB intervals and 2MB intervals must be multiply of 2MB

     * @param configurationList : list of intervals to validate
     * @return validate result
     */
    ValidatorErrorMessage Validate(MemoryIntervalList& configurationList);
private:
    ValidatorErrorMessage validateIntervalsOf(MemoryIntervalList &intervals) const;
};

#endif //MOSALLOC_MEMORYINTERVALSVALIDATOR_H
