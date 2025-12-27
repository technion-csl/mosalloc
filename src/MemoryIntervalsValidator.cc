//
// Created by User on 17/11/2019.
//

#include "MemoryIntervalsValidator.h"

ValidatorErrorMessage MemoryIntervalsValidator::Validate(MemoryIntervalList& configurationList) {
    MemoryIntervalList intervalsOf1GB;
    MemoryIntervalList intervalsOf2MB;
    configurationList.Sort();
    configurationList.CopyMemoryIntervalsOf1GBTo(intervalsOf1GB);
    configurationList.CopyMemoryIntervalsOf2MBTo(intervalsOf2MB);
    if(intervalsOf1GB.GetLength() + intervalsOf2MB.GetLength() != configurationList.GetLength()) {
        return ValidatorErrorMessage::INVALID_PAGE_SIZE;
    }
    auto res1GB = validateIntervalsOf(intervalsOf1GB);
    if(res1GB != ValidatorErrorMessage::SUCCESS)
        return res1GB;
    auto res2MB = validateIntervalsOf(intervalsOf2MB);
    if(res2MB != ValidatorErrorMessage::SUCCESS)
        return res2MB;
    if(intervalsOf1GB.GetLength() != 0 && intervalsOf2MB.GetLength() != 0) {

        if(!IS_ALIGNED((intervalsOf1GB.At(0)._start_offset - intervalsOf2MB.At(0)._start_offset), PageSize::HUGE_2MB)) {
            return ValidatorErrorMessage::OFFSET_BETWEEN_1GB_AND_2MB_INTERVALS_ERROR;
        }
    }
    return ValidatorErrorMessage::SUCCESS;

}


ValidatorErrorMessage MemoryIntervalsValidator::validateIntervalsOf (MemoryIntervalList &intervals) const {
    if(intervals.GetLength() != 0) {
        const PageSize pageSize = intervals.At(0)._page_size;
        intervals.Sort();
        for(unsigned int i = 0; i < intervals.GetLength(); i++) {
            if(!IS_ALIGNED(intervals.At(i)._start_offset, PageSize::BASE_4KB)) {
                return (pageSize == PageSize::HUGE_1GB) ?
                       ValidatorErrorMessage::INVALID_1GB_START_OFFSET :
                       ValidatorErrorMessage::INVALID_2MB_START_OFFSET;
            }
            auto size_of_interval =
                    intervals.At(i)._end_offset - intervals.At(i)._start_offset;
            if(size_of_interval <= 0 || !IS_ALIGNED(size_of_interval, pageSize)) {
                return (pageSize == PageSize::HUGE_1GB) ?
                       ValidatorErrorMessage::SIZE_OF_1GB_INTERVAL_ERROR :
                        ValidatorErrorMessage::SIZE_OF_2MB_INTERVAL_ERROR;
            }
            if(i == 0)
                continue;
            auto offset_between_intervals =
                    intervals.At(i)._start_offset - intervals.At(i-1)._end_offset;
            if (!IS_ALIGNED(offset_between_intervals, pageSize)) {
                return (pageSize == PageSize::HUGE_1GB) ?
                        ValidatorErrorMessage::OFFSET_BETWEEN_TWO_1GB_INTERVALS_ERROR :
                        ValidatorErrorMessage::OFFSET_BETWEEN_TWO_2MB_INTERVALS_ERROR;
            }
        }
    }
    return ValidatorErrorMessage::SUCCESS;
}
